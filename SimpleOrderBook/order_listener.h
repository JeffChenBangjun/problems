#ifndef ORDER_LISTENER_H
#define ORDER_LISTENER_H

#include <unordered_map>
#include <deque>
#include <chrono>


///\class OrderListener
/// Holding order data including price, quantity, side and id
class Order
{
public:
	Order(
		int id,
		char side,
		double price,
		int quantity
		);

	double GetPrice() const { return mPrice; }
	int GetId() const { return mId; }
	int GetQuantity() const { return mQuantity; }
	char GetSide() const { return mSide; }
	void SetQuantity(int val) { mQuantity = val; }
	
private:
	double mPrice;
	int mId;
	int mQuantity;
	char mSide;
};

///\class OrderListener
///  client order tracking module for a single symbol for a simple market that only supports order insertion and replacement (no deletion)
/// OrderListener Track the request rate. OrderListener can be instantiated with a number of
/// requests X and a number of seconds Y, and support the following operations :
///  1. Have more than X requests(from OnInsertOrderRequest and / or OnReplaceOrderRequest) been received in the last Y seconds ?
///  2. How long(in fractions of a seconds) must one wait until a request could be submitted without(1) returning true afterwards ? 
///  (Return 0.0 if the request could be submitted right away.)
///	 OrderListener maintains the following information: 
/// Net Filled Quantity(NFQ).Return the sum of all fill quantities where filled bids count positively and filled offers count negatively.
/// Confirmed Order Value(COV).Given a side, return the total value of all orders on that side of the market, defined as the product of the
/// price and quantity of all orders that have been acknowledged but not fully filled or replaced.
/// Pending Order Value(POV).Given a side, return the minimum and maximum possible total values of all orders on that side of the
/// market, taking into account pending requests that have not yet been acknowledged.
/// Please note this class assumes all the operations are invoked the same thread context
class OrderListener {
public:
	// constructor takes maximum number of requests allowed in a period and a period sepecified in a number of seconds
	OrderListener(int noOfRequests, int noOfSeconds);

	// does not allow copying
	OrderListener(const OrderListener&) = delete;
	OrderListener & operator=(const OrderListener&) = delete;

	// Indicates the client has sent a new order request to the market. Exactly one
	// callback will follow:
	// * 'OnRequestAcknowledged', in which case order 'id' is active in the market; or
	// * 'OnRequestRejected', in which case the order was never active in the market.
	virtual void OnInsertOrderRequest(
		int id,
		char side,// 'B' for bid, 'O' for offer
		double price,
		int quantity
		);

	// Indicates the client has sent a request to change the quantity of an order.
	// Exactly one callback will follow:
	// * 'OnRequestAcknowledged', in which case the order quantity was modified and the
	// order is now tracked by ID 'newId'; or
	// * 'OnRequestRejected', in which case the order was not modified and remains
	// tracked by ID 'oldId'.
	virtual void OnReplaceOrderRequest(
		int oldId,	// The existing order to modify
		int newId,	// The new order ID to use if the modification succeeds
		int deltaQuantity // How much the quantity should be increased/decreased
		);

	 // These three callbacks represent market confirmations.
	 // Indicates the insert or modify request was accepted. 
	virtual void OnRequestAcknowledged(int id);

	 // Indicates the insert or modify request was rejected.
	virtual void OnRequestRejected(int id);

	// Indicates that the order quantity was reduced (and filled) by 'quantityFilled'. 
	virtual void OnOrderFilled(int id, int quantityFilled);

	// log Net Filled Quantity, Confirmed Order Value and Pending Order Value (POV)
	void PrintQuantity() const;

	// returns true if more than mNoOfRequests requests(from OnInsertOrderRequest and / or OnReplaceOrderRequest) been received in the last mNoOfSeconds seconds 
	// returns false otherwise
	bool IsRateExceeded();

	// returns how long (in fractions of a seconds) must one wait until a request could be submitted without IsRateExceeded returning true afterwards? (Return 0.0 if
	//the request could be submitted right away.)
	double TimeToWait();

private:
	// Add timestamp for new order and delete old timestamps
	void UpdateRequestRate(); 

	// Delete all order timestamps which are more than mNoOfSeconds away from given currentTime
	void DeleteOldTimeStamps(const std::chrono::time_point<std::chrono::steady_clock>& currentTime);

	int mNoOfRequests; // maximum number of order requests allowed in a period
	int mNoOfSeconds; // a period sepecified in a number of seconds
	typedef std::deque< std::chrono::time_point<std::chrono::steady_clock>> OrderTimeQueue;
	OrderTimeQueue mOrderTimeQueue; // double ended queue to record order timestamps in order
	typedef std::unordered_map<int, Order> OrderMap;
	OrderMap mOrderMap;
	long long mNetFilledQuantity;
	double mBidConfirmedOrderValue;
	double mOfferConfirmedOrderValue;
	double mMinBidPendingOrderValue;
	double mMinOfferPendingOrderValue;
	double mMaxBidPendingOrderValue;
	double mMaxOfferPendingOrderValue;
	typedef std::unordered_map<int, int> OrderModificationMap;
	OrderModificationMap mOrderModificationMap;
};

#endif
