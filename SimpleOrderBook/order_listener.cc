#include "order_listener.h"
#include <assert.h>
#include <iostream>
#include <thread>

namespace
{
	const char BID_SIDE = 'B';
	const char OFFER_SIDE = 'O';
}

Order::Order(
	int id,
	char side,
	double price,
	int quantity
	)
:	mPrice(price),
	mId(id),
	mQuantity(quantity),
	mSide(side)
{

}

OrderListener::OrderListener(int noOfRequests, int noOfSeconds)
:	mNoOfRequests(noOfRequests),
	mNoOfSeconds(noOfSeconds),
	mNetFilledQuantity(0),
	mBidConfirmedOrderValue(0.0),
	mOfferConfirmedOrderValue(0.0),
	mMinBidPendingOrderValue(0.0),
	mMinOfferPendingOrderValue(0.0),
	mMaxBidPendingOrderValue(0.0),
	mMaxOfferPendingOrderValue(0.0)
{

}

double OrderListener::TimeToWait()
{
	if (IsRateExceeded() == false)
	{
		return 0.0;
	}
	int pos = static_cast<int>(mOrderTimeQueue.size() - mNoOfRequests);
	auto currentTime = std::chrono::steady_clock::now();
	double elapsedSeconds = ((currentTime - mOrderTimeQueue[pos]).count()) * std::chrono::steady_clock::period::num /
		static_cast<double>(std::chrono::steady_clock::period::den);

	return mNoOfSeconds - elapsedSeconds;
}

bool OrderListener::IsRateExceeded()
{
	auto currentTime = std::chrono::steady_clock::now();
	DeleteOldTimeStamps(currentTime);
	std::cout << mOrderTimeQueue.size() << " requests has been received in last " << mNoOfSeconds << " Seconds" << std::endl;
	if (mOrderTimeQueue.size() > mNoOfRequests)
	{
		return true;
	}

	return false;
}

void OrderListener::DeleteOldTimeStamps(const std::chrono::time_point<std::chrono::steady_clock>& currentTime)
{
	bool existsOldTimeStamps = true;	
	while (existsOldTimeStamps && !mOrderTimeQueue.empty())
	{
		auto oldTime = mOrderTimeQueue.front();
		double elapsedSeconds = ((currentTime - oldTime).count()) * std::chrono::steady_clock::period::num / 
			static_cast<double>(std::chrono::steady_clock::period::den);
		if (elapsedSeconds > mNoOfSeconds)
		{
			mOrderTimeQueue.pop_front();
		}
		else
		{
			existsOldTimeStamps = false;
		}
	}
}

void OrderListener::UpdateRequestRate()
{
	auto currentTime = std::chrono::steady_clock::now();
	DeleteOldTimeStamps(currentTime);
	mOrderTimeQueue.push_back(currentTime);
}

void OrderListener::PrintQuantity() const
{
	std::cout << "Net Filled Quantity:" << mNetFilledQuantity
		 << "|BidCOF:" << mBidConfirmedOrderValue
		 << "|OfferCOF:" << mOfferConfirmedOrderValue
		 << "|BidMinPOF:" << mMinBidPendingOrderValue
		 << "|BidMaxPOF:" << mMaxBidPendingOrderValue
		 << "|OfferMinPOF:" << mMinOfferPendingOrderValue
		 << "|OfferMaxPOF:" << mMaxOfferPendingOrderValue << std::endl;
}

void OrderListener::OnInsertOrderRequest(
	int id,
	char side,
	double price,
	int quantity
	)
{
	UpdateRequestRate();
	Order order(
		id,
		side,
		price,
		quantity
		);
	mOrderMap.insert(std::make_pair(id, order));
	double value = price * quantity;
	assert(side == BID_SIDE || side == OFFER_SIDE);
	if (side == BID_SIDE)
	{
		mMaxBidPendingOrderValue += value;
	}
	else
	{
		mMaxOfferPendingOrderValue += value;
	}
}
	
void OrderListener::OnReplaceOrderRequest(
	int oldId,
	int newId,
	int deltaQuantity)
{
	UpdateRequestRate();
	auto iter = mOrderMap.find(oldId);
	if (iter == mOrderMap.end())
	{
		std::cerr << "[OnReplaceOrderRequest] OrderId " << oldId << "not found" << std::endl;
		assert(false);
		return;
	}

	double price = iter->second.GetPrice();
	double value = deltaQuantity * price;
	char side = iter->second.GetSide();
	if (side == BID_SIDE)
	{
		mMaxBidPendingOrderValue += value;
	}
	else if (side == OFFER_SIDE)
	{
		mMaxOfferPendingOrderValue += value;
	}

	mOrderModificationMap[newId] = oldId;
	Order newOrder(newId, side, price, deltaQuantity);
	mOrderMap.insert(std::make_pair(newId, newOrder));
}
	
void OrderListener::OnRequestAcknowledged(int id)
{
	auto iter = mOrderMap.find(id);
	if (iter == mOrderMap.end())
	{
		// not expected
		std::cerr << "[OnRequestAcknowledged] OrderId " << id << "not found" << std::endl;
		assert(false);
		return;
	}

	double value = iter->second.GetQuantity() * iter->second.GetPrice();
	char side = iter->second.GetSide();
	if (side == BID_SIDE)
	{
		mBidConfirmedOrderValue += value;
		mMinBidPendingOrderValue += value;
	}
	else if (side == OFFER_SIDE)
	{
		mOfferConfirmedOrderValue += value;
		mMinOfferPendingOrderValue += value;
	}
	auto iterOrderModification = mOrderModificationMap.find(id);
	if (iterOrderModification != mOrderModificationMap.end())
	{
		// new order is Acknowledged, so order is tracked by new id
		// delete the order tracked by old id
		int oldId = iterOrderModification->second;
		mOrderModificationMap.erase(iterOrderModification);
		auto iterToDel = mOrderMap.find(oldId);
		if (iterToDel != mOrderMap.end())
		{
			mOrderMap.erase(iterToDel);
		}		
	}
}

void OrderListener::OnRequestRejected(int id)
{
	auto iter = mOrderMap.find(id);
	if (iter == mOrderMap.end())
	{
		// not expected
		std::cerr << "[OnRequestRejected] OrderId " << id << "not found" << std::endl;
		assert(false);
		return;
	}

	double value = iter->second.GetQuantity() * iter->second.GetPrice();
	char side = iter->second.GetSide();
	if (side == BID_SIDE)
	{
		mMaxBidPendingOrderValue -= value;
	}
	else if (side == OFFER_SIDE)
	{
		mMaxOfferPendingOrderValue -= value;
	}

	mOrderMap.erase(iter);

	auto iterOrderModification = mOrderModificationMap.find(id);
	if (iterOrderModification != mOrderModificationMap.end())
	{
		// order still tracked by old id, delete the order tracked by new id
		mOrderModificationMap.erase(iterOrderModification);
		auto iterToDel = mOrderMap.find(id);
		if (iterToDel != mOrderMap.end())
		{
			mOrderMap.erase(iterToDel);
		}		
	
	}
}

void OrderListener::OnOrderFilled(int id, int quantityFilled)
{
	auto iter = mOrderMap.find(id);
	if (iter == mOrderMap.end())
	{
		// not expected
		std::cerr << "[OnOrderFilled] OrderId " << id << "not found" << std::endl;
		assert(false);
		return;
	}
	assert(quantityFilled <= iter->second.GetQuantity());
	double value = quantityFilled * iter->second.GetPrice();
	char side = iter->second.GetSide();
	if (side == BID_SIDE)
	{
		mNetFilledQuantity += quantityFilled;
		mMinBidPendingOrderValue -= value;
		mMaxBidPendingOrderValue -= value;
		mBidConfirmedOrderValue -= value;
	}
	else if (side == OFFER_SIDE)
	{
		mNetFilledQuantity -= quantityFilled;
		mMinOfferPendingOrderValue -= value;
		mMaxOfferPendingOrderValue -= value;
		mOfferConfirmedOrderValue -= value;
	}

	iter->second.SetQuantity(iter->second.GetQuantity() - quantityFilled);
	if (iter->second.GetQuantity() == 0)
	{
		mOrderMap.erase(iter);
	}
}

int main()
{
	/* Testing code for part A requst rate*/
	OrderListener orderListener1(3, 4);
	orderListener1.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	orderListener1.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	using namespace std::literals;
	orderListener1.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	std::this_thread::sleep_for(1s);
	orderListener1.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	std::cout << "IsRateExceeded:" << orderListener1.IsRateExceeded() << std::endl;
	std::cout << orderListener1.TimeToWait() << std::endl;

	OrderListener orderListener2(3, 2);
	orderListener2.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	orderListener2.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	orderListener2.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	std::this_thread::sleep_for(3s);
	orderListener2.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	std::cout << "IsRateExceeded:" << orderListener2.IsRateExceeded() << std::endl;
	std::cout << orderListener2.TimeToWait() << std::endl;


	/* Testing code for part B*/
	OrderListener orderListener3(2, 3);
	orderListener3.OnInsertOrderRequest(1, BID_SIDE, 10.0, 10);
	orderListener3.PrintQuantity();
	orderListener3.OnRequestAcknowledged(1);
	orderListener3.PrintQuantity();
	orderListener3.OnInsertOrderRequest(2, OFFER_SIDE, 15.0, 25);
	orderListener3.PrintQuantity();
	orderListener3.OnRequestAcknowledged(2);
	orderListener3.PrintQuantity();
	orderListener3.OnOrderFilled(1, 5);
	orderListener3.PrintQuantity();
	orderListener3.OnOrderFilled(1, 5);
	orderListener3.PrintQuantity();
	orderListener3.OnReplaceOrderRequest(2, 3, 10);
	orderListener3.PrintQuantity();
	orderListener3.OnOrderFilled(2, 25);
	orderListener3.PrintQuantity();
	orderListener3.OnRequestRejected(3);
	orderListener3.PrintQuantity();

	return 0;
}

