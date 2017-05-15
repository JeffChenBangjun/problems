/*
Crack-free Walls
https://projecteuler.net/problem=215

Consider the problem of building a wall out of 2×1 and 3×1 bricks (horizontal×vertical dimensions) such that, for extra strength, the gaps between horizontally-adjacent bricks never line up in consecutive layers, i.e. never form a "running crack".
For example, the following 9×3 wall is not acceptable due to the running crack shown in red:
There are eight ways of forming a crack-free 9×3 wall, written W(9,3) = 8.

Calculate W(32,10).
*/

#include <vector>
#include <set>
#include <iostream>



const int BRICK_WIDTH_SMALL = 2;        // 2 * 1 brick has width 2
const int BRICK_WIDTH_BIG = 3;               // 3 * 1 brick has width 3





/**
* This is checking if two given layers have brick position lined up
*
* @param [in] layerPos1  a list of brick position for a layer
* @param [in] layerPos2  a list of brick position for a layer
*
* @retval true two given layer have brick position lined up
* @retval false two given layer have no brick position lined up
*/
bool IsHavingCrack(const std::vector<int>& layerPos1, const std::vector<int>& layerPos2)
{
	auto&& iter1 = layerPos1.begin();
	auto&& iter2 = layerPos2.begin();
	auto&& iterLast1 = layerPos1.end();
	auto&& iterLast2 = layerPos2.end();
	while (iter1 != iterLast1 && iter2 != iterLast2)
	{
		if (*iter1 < *iter2)
		{
			++iter1;
		}
		else if (*iter2 < *iter1)
		{
			++iter2;
		}
		else
		{
			return true;
		}
	}

	return false;
}



/**
* This generate all possible compatibale layers satifiy the following condition
* "the gaps between horizontally-adjacent bricks never line up in consecutive layers"
*
* @param [in] allLayers  All possible combinations for one layer
* @param [out] compatibleLayers  Look up array matrix containing all compatible layers
*/
void GenerateCompatibleLayers(const std::vector<std::vector<int>>& allPossibleLayers, std::vector<std::vector<int>>& compatibleLayers)
{
	unsigned long long lenOfLayers = allPossibleLayers.size();
	std::vector<int> empty;
	compatibleLayers.resize(lenOfLayers, empty);
	for (int i = 0; i < lenOfLayers; ++i)
	{
		for (int j = i + 1; j < lenOfLayers; ++j)
		{
			if (IsHavingCrack(allPossibleLayers[i], allPossibleLayers[j]) == false)
			{
				compatibleLayers[i].push_back(j);
				compatibleLayers[j].push_back(i);
			}
		}
	}
}


/**
* This calculate maximum ways of constructing a crack free walls
* The idea is assuming you have computed all possible ways for f(height - 1)
* Using compatiblelayers, f(height) can be computed
*
* @param [in] allLayers  All possible combinations for one layer
* @param [in] height  Height of the wall
* @param [in] compatibleLayers  Compatible layer arrary matrix
*
* @retval maximum ways of constructing a crack free walls with given width and height
*/
unsigned long long Calculate(const std::vector<std::vector<int>>& allLayers, int height, const std::vector<std::vector<int>>& compatibleLayers)
{
	std::vector<unsigned long long> layer;
	layer.resize(allLayers.size(), 0);
	std::vector<std::vector<unsigned long long>> layers(height, layer);

	for (int h = 0; h < height; ++h)
	{
		if (h == 0)
		{
			for (unsigned int i = 0; i < allLayers.size(); ++i)
			{
				layers[h][i] = 1;
			}
		}
		else
		{
			const std::vector<unsigned long long>& layerUnder = layers[h - 1];
			for (int i = 0; i < layerUnder.size(); ++i)
			{
				if (layerUnder[i] > 0)
				{
					for (auto&& j : compatibleLayers[i])
					{
						layers[h][j] += layerUnder[i];
					}
				}
			}
		}
	}
	unsigned long long sum = 0;
	for (auto&& i : layers[height - 1])
	{
		sum += i;
	}

	return sum;
}

/**
* This append one brick to each layer of given list of layers. Brick is tracked by its absolute horizontal position
*
* @param [in] wallWidth  Width of the wall
* @param [in] brickWidth  Width of the brick to append
* @param [in/out] list list of possible layers, a brick need to be appended to each layer
*
* @retval list of possbile combination for a layer
*/
void AppendBrick(int wallWidth, int brickWidth, std::vector<std::vector<int>>& list)
{
	for (auto&& v : list)
	{
		int pos = 0;
		if (!v.empty())
		{
			pos = v.back();
		}
		pos += brickWidth;
		// End of wall position not added, as this position is considered in generating compatible layer
		if (pos != wallWidth)
		{
			v.push_back(pos);
		}
	}
}

/**
* This generates all possible combinations of bricks for a given width
*
* @param [in] width  Width of the wall
*
* @retval list of possbile combination for a layer
*/
std::vector<std::vector<int>> GenerateAllPossibleLayers(int width)
{
	if (width < BRICK_WIDTH_SMALL)
	{
		return std::vector<std::vector<int>>();
	}
	if (width <= BRICK_WIDTH_BIG)
	{
		return std::vector<std::vector<int>>(1, std::vector<int>(1, width));
	}
	std::vector<std::vector<std::vector<int>>> allLayers(
		width + 1, std::vector<std::vector<int>>());
	allLayers[BRICK_WIDTH_SMALL] =
		std::vector<std::vector<int>>(1, std::vector<int>(1, BRICK_WIDTH_SMALL));
	allLayers[BRICK_WIDTH_BIG] = 
		std::vector<std::vector<int>>(1, std::vector<int>(1, BRICK_WIDTH_BIG));
	for (int i = BRICK_WIDTH_BIG + 1; i < allLayers.size(); ++i)
	{
		auto layerS = allLayers[i - BRICK_WIDTH_SMALL];//f(w - 2)
		auto layerL = allLayers[i - BRICK_WIDTH_BIG];//f(w - 3)
		AppendBrick(width, BRICK_WIDTH_SMALL, layerS);
		AppendBrick(width, BRICK_WIDTH_BIG, layerL);
		allLayers[i].reserve(layerS.size() + layerL.size());
		allLayers[i].insert(allLayers[i].end(), layerS.begin(), layerS.end());
		allLayers[i].insert(allLayers[i].end(), layerL.begin(), layerL.end());
	}

	return allLayers[width];
}

int main()
{
	int width = 32;
	int height = 10;

	std::vector<std::vector<int>> allPossiblelayers = GenerateAllPossibleLayers(width);
	std::vector<std::vector<int>> compatibleLayers;
	GenerateCompatibleLayers(allPossiblelayers, compatibleLayers);
	unsigned long long count = Calculate(allPossiblelayers, height, compatibleLayers);
	std::cout << count << std::endl;

	return 0;
}