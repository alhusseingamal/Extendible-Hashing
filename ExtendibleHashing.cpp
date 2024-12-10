
#include "ExtendibleHashing.h"
#include <bitset>
#include <iostream>
#include <cmath>
#include <string>
using namespace std;


void displayNotFound(int key) {
	std::cout << "\t\t\t key:  " << key << " \t value : \t not found" << std::endl;
}

//Display one record entry
void displayItem(DataItem* dataItem) {
	if (dataItem != nullptr && dataItem->valid)
		std::cout << "\t\t\t key: " << std::bitset<8>(dataItem->key) << "\t value:\t" << dataItem->data << std::endl;
	else
		std::cout << "\t\t\t key:  ============ \t value:\t empty data" << std::endl;
}

void displayBucket(Bucket& currentBucket, string& depths, string& values, int verbose) {
	depths.append(std::to_string(currentBucket.localDepth));
	depths.append(",");
	values.append("[");
	if (verbose)
		std::cout << "\t\tBucket:\t local depth:" << currentBucket.localDepth << std::endl;
	if (currentBucket.currentEntries == 0)
	{
		if (verbose)
			std::cout << "\t\t\tNo Data yet\n";
		for (int i = 0; i < RECORDSPERBUCKET; i++)
		{
			values.append("null");
			values.append(",");
		}
	}
	else {
		for (int i = 0; i < RECORDSPERBUCKET; i++)
		{
			if (currentBucket.dataItem[i].valid)
			{
				values.append(std::to_string(currentBucket.dataItem[i].data));
				values.append(",");
				if (verbose)
					std::cout << "\t\t\t key: " << std::bitset<8>(currentBucket.dataItem[i].key) << "\t value:\t" << currentBucket.dataItem[i].data << std::endl;
			}
			else {
				values.append("null");
				values.append(",");
				if (verbose)
					std::cout << "\t\t\t key:  ============ \t value:\t empty data" << std::endl;
			}

		}
	}
	values.pop_back();
	values.append("]");
}

void displayDirectory(GlobalDirectory& globaldirectory, Bucket& currentBucket, int verbose) {
	std::cout << "Directory:\t global depth:" << globaldirectory.globalDepth << std::endl;
	string values = "(";
	string depths = "(";
	int count = 0;
	//string locations = "(";
	if (globaldirectory.length == 0)
	{
		count++;
		std::cout << "\tNo Directory yet\n";
		displayBucket(currentBucket, depths, values, verbose);
	}
	else
	{
		for (int i = 0; i < globaldirectory.length; i++)
		{
			if (i == 0)
			{
				count++;
			}
			else {
				if (globaldirectory.entry[i - 1] != globaldirectory.entry[i])
					count++;
			}
			if (verbose)
				std::cout << "\t key: " << std::bitset<8>(i) << "\t value:\t" << globaldirectory.entry[i] << std::endl;
			displayBucket(*globaldirectory.entry[i], depths, values, verbose);
			if (verbose)
				std::cout << "-----------------------------------------------\n\n";
		}
		//values.pop_back();
		depths.pop_back();
	}

	values.append(")");
	depths.append(")");
	std::cout << " buckets:\t" << count << "/" << globaldirectory.length << endl;
	std::cout << "values:\t" << values << endl;
	std::cout << "depths:\t" << depths << endl;
	std::cout << "=========================\n";
	char t[100];
}

// Hashing function and getting directory Index
int getCurrentHash(int key, int depth) {
	int hashedKey = (key & MAXKEYVALUE) >> (MAXKEYLENGTH - depth);
		return hashedKey;
}


// try to insert item into a bucket
// The re-organization after deletion ensures that the first currentEntries (from 0 to currentEntries - 1) are full
// and that we can insert at the position currentEntries
int insertItemIntoBucket(Bucket& currentBucket, DataItem data)
{
	if (currentBucket.currentEntries < RECORDSPERBUCKET)
	{
		currentBucket.dataItem[currentBucket.currentEntries++] = data;
		return 1;
	}
	return 0;
}

// look for an item in a bucket using key, if found call displayItem(..), if not found call displayNotFound()
void findItemInBucket(Bucket& currentBucket, int key)
{
	for (int i = 0; i < currentBucket.currentEntries; i++)
	{
		if (currentBucket.dataItem[i].key == key)
		{
			displayItem(&currentBucket.dataItem[i]);
			return;
		}
	}
	displayNotFound(key);
	return;
}

// try to Delete item based on a key value from a bucket
// reorganize the bucket to remove gaps resulting from deleted items(actually it's only one deleted item because we clean up after each deletion)
int deleteItemFromBucket(Bucket& currentBucket, int key)
{
	for (int i = 0; i < currentBucket.currentEntries; i++)
	{
		if (currentBucket.dataItem[i].key == key)
		{
			currentBucket.dataItem[i] = DataItem();	// the default constructor of DataItem() zeros out all entries, effectively deleting the item
			currentBucket.currentEntries--;
			// Bucket Reorganization
			int lastEmptyLocation = i;
			for(int j = i + 1; j < RECORDSPERBUCKET; j++)
			{
				if(currentBucket.dataItem[j].valid)
				{
					currentBucket.dataItem[lastEmptyLocation++] = currentBucket.dataItem[j];
					currentBucket.dataItem[j] = DataItem();
				}
				else	// we can stop here because once we meet an invalid entry we know that we have reached the end of all valid entries
				{		// there cannot be other later invalid entries since we perform re-organization after each deletion
					break;
				}
			}
			return 1;
		}
	}
	return 0;
}

// Bit Manipulation functions

// function to get the values resulting from a split. For example, 00 is split into 000 and 001
pair<int, int> calculateSplits(int num)
{
	return make_pair(num << 1, (num << 1) + 1);
}

// get the Split Image: if last bit is 0, then the peer has a 1 in the last bit, and vice versa
int getSplitImage(int num)
{
	if(num & 1 == 1)	// last bit is 1, so we return the complement
		return num - 1;
	else
		return num + 1;
}

// try to insert item in the file, if the bucket is full, extend the directory,
// if you extended the directory five times but it still doesn't work, return 0
// note 1: to illustrate: assume globalDepth = 5, localDepth = 3. All the entries at indices 001XX should point to the bucket with
// BucketIndex = 001; the amount of right shift needed here is the depth difference (in this case 5-3=2)

int insertItem(DataItem data, Bucket& currentBucket, GlobalDirectory& globaldirectory)
{
	if (globaldirectory.globalDepth == 0) {	// No directory yet
		bool insertion_success = insertItemIntoBucket(currentBucket, data);
		if (insertion_success == 0)	// bucket (the only one till now) is full
			createFirstTimeDirectory(globaldirectory, currentBucket);	// a directory is created, we will try to insert again later
		else // bucket is not yet full, insertion was successful, so no need for a directory
			return 1;
	}
	while(true)
	{
		int hashKey = getCurrentHash(data.key, globaldirectory.globalDepth);
		if (insertItemIntoBucket(*globaldirectory.entry[hashKey], data) == 0)	// bucket is full -> insertion failed
		{
			if (globaldirectory.entry[hashKey]->localDepth < globaldirectory.globalDepth)	// -> split the bucket
			{
				// Bucket* oldBucket = globaldirectory.entry[hashKey]; 	// pointer to the old bucket
				Bucket oldBucket = *globaldirectory.entry[hashKey]; 	// auxiliary variable to hold old bucket
				int newLocalDepth = globaldirectory.entry[hashKey]->localDepth + 1;
				// create splits
				Bucket* firstSplit = new Bucket(newLocalDepth);
				Bucket* secondSplit = new Bucket(newLocalDepth);
				int temp = (hashKey >> (globaldirectory.globalDepth - (newLocalDepth - 1)));
				pair<int, int> splitValues = calculateSplits(temp);

				// place the items in oldBucket in the appropriate bucket from the two newly created Buckets
				for (auto item : oldBucket.dataItem) // we only need to loop on the 1st currentEntries buckets as we are sure those are the valid ones
				{
					if(item.valid)
					{
						int hashIndex = getCurrentHash(item.key, newLocalDepth);
						if(hashIndex == splitValues.first)
							insertItemIntoBucket(*firstSplit, item);
						else if(hashIndex == splitValues.second)
							insertItemIntoBucket(*secondSplit, item);
					}
				}

				// delete the old bucket
				delete [] globaldirectory.entry[hashKey];

				// modify the entries/pointers in the directory to point to the new buckets
				for(int j = 0; j < globaldirectory.length; j++)
				{
					int BucketIndex = (j >> (globaldirectory.globalDepth - newLocalDepth));	// see note 1 above this function for illustration
					if (BucketIndex == splitValues.first)
						globaldirectory.entry[j] = firstSplit;
					else if (BucketIndex == splitValues.second)
						globaldirectory.entry[j] = secondSplit;
				}
			}
			else	// if localDepth equals globalDepth, we need to extend Directory
			{
				if(globaldirectory.globalDepth == MAXKEYLENGTH)	// we can extend the directory as long as the globalDepth is < MAXKEYLENGTH
					return 0;
				else
					extendDirectory(globaldirectory, hashKey);
			}
		}
		else return 1;	// successful insertion
	}
	return 0;
}

// search the directory for an item using the key
void searchItem(int key, Bucket& currentBucket, GlobalDirectory& globaldirectory)
{
	if (globaldirectory.globalDepth == 0)
		findItemInBucket(currentBucket, key);
	else
	{
		int hashKey = getCurrentHash(key, globaldirectory.globalDepth);
		findItemInBucket(*globaldirectory.entry[hashKey], key);
	}
}

// search on an item based on the key and delete it.
// in case the whole bucket is empty, the bucket should be merged again with its peer and the pointer should point to the peer bucket
// in case of delete success, we call checkDirectoryMinimization to compress directory if needed.
// we don't just call checkDirectoryMinimization once. we loop on it to continue the merging step
// as there might be buckets that are already empty but couldn't be remerged before, so, if possible, we remerge them with the newly
// emptied buckets (if any)

int deleteItem(int key, Bucket& currentBucket, GlobalDirectory& globaldirectory)
{
	if (globaldirectory.globalDepth == 0)
		return deleteItemFromBucket(currentBucket, key);
	else
	{
		int hashKey = getCurrentHash(key, globaldirectory.globalDepth);
		int delete_success = deleteItemFromBucket(*globaldirectory.entry[hashKey], key);
		if(delete_success == 0)	// may fail to delete if the item doesn't exist
			return 0;
		else	// check empty bucket remerging after item deletion
		{
			while (true)
			{
				bool flag = false;	// a flag to indicate whether any remerging happened in the current iteration
				for(int i = 0; i < globaldirectory.length; i++)
				{
					int BucketIndex = (i >> (globaldirectory.globalDepth - globaldirectory.entry[i]->localDepth));
					int BucketPeerIndex = getSplitImage(BucketIndex) << (globaldirectory.globalDepth - globaldirectory.entry[i]->localDepth);
					// remerging happens when a bucket is empty, and a peer of the same localDepth is found
					// The 2nd condition is necessary because otherwise, for example, 0001 may be returned as a peer to 001
					if (globaldirectory.entry[i]->currentEntries == 0 && 
					globaldirectory.entry[i]->localDepth == globaldirectory.entry[BucketPeerIndex]->localDepth)
					{
						flag = true;
						delete[] globaldirectory.entry[i];
						globaldirectory.entry[i] = globaldirectory.entry[BucketPeerIndex];
						globaldirectory.entry[i]->localDepth = --globaldirectory.entry[BucketPeerIndex]->localDepth; // localDepth of both remerged buckets is decremented
						checkDirectoryMinimization(globaldirectory);
					}
				}
				if (flag == false)
					break;	// if we have made a full iteration without remerging any buckets, we stop as we won't make in any further iterations
			}
			return 1;
		}
	}
	return 0;
}

// create  the first directory, this might help you to implement extendDirectory
int createFirstTimeDirectory(GlobalDirectory& globaldirectory, Bucket& currentBucket)
{
	globaldirectory.globalDepth = 1;
	globaldirectory.length = 2;
	globaldirectory.entry = new Bucket * [globaldirectory.length];	// globaldirectory.entry is a pointer to an array of "globaldirectory.length" pointers to buckets
	globaldirectory.entry[0] = new Bucket(globaldirectory.globalDepth);	// initialize the 1st bucket
	globaldirectory.entry[1] = new Bucket(globaldirectory.globalDepth); // initialize the 2nd bucket
	for (int i = 0; i < RECORDSPERBUCKET; i++) {
		int hashKey = getCurrentHash(currentBucket.dataItem[i].key, globaldirectory.globalDepth);
		if (hashKey == -1)
		{
			return -1;
		}
		insertItemIntoBucket(*globaldirectory.entry[hashKey], currentBucket.dataItem[i]);
	}
	return 1;
}

// expand the directory because we can't find a space anymore in the file
int extendDirectory(GlobalDirectory &globaldirectory, int splitIndex)
{
	if(globaldirectory.globalDepth == MAXKEYLENGTH) // check if directory can't be extended further
		return 0;
	
	Bucket* *tempBucketList = new Bucket * [globaldirectory.length << 1];	// auxiliary pointer to the new directory of double the size
	
	for(int i = 0; i < globaldirectory.length; i++)
	{ // for illustration: after extending a directory, 01 becomes both 010 and 011, both of them should point to the bucket pointed to by 01
		pair<int, int> splitValues = calculateSplits(i);
		tempBucketList[splitValues.first] = globaldirectory.entry[i];
		tempBucketList[splitValues.second] = globaldirectory.entry[i];
	}
	// delete the old list of bucket pointers to avoid memory leakage
	delete[] globaldirectory.entry;
	globaldirectory.entry = tempBucketList;	// directory entry is now the newly created list
	// length is doubled as globalDepth becomes one bit larger
	globaldirectory.length <<= 1;
	globaldirectory.globalDepth++;
	return 1;
}

// If all bucket local depths are less than the global depth, compress the directory depth by one 
// loop on this function as appropriate to compress the file back
int checkDirectoryMinimization(GlobalDirectory& globaldirectory)
{
	// if one bucket local depth is as large as the global depth, we cannot compress
	for (int i = 0; i < globaldirectory.length; i++)
		if (globaldirectory.entry[i]->localDepth == globaldirectory.globalDepth)
			return -1;
	
	// if its depth = 1, we cannot compress it further, so we stop
	if (globaldirectory.length == 1)
		return -1;
	
	// all are smaller than localdepth, initiate minimization
	// this is actually too stupid, it should be at least half empty to initiate minimization
	// but we will keep it like that for simplicity
	int oldGlobalDepth, oldLength;
	Bucket** oldEntry;

	oldGlobalDepth = globaldirectory.globalDepth;
	oldLength = globaldirectory.length;
	globaldirectory.globalDepth -= 1;
	globaldirectory.length /= 2;
	oldEntry = globaldirectory.entry;
	globaldirectory.entry = new Bucket * [globaldirectory.length];
	for (int i = 0; i < globaldirectory.length; i++) {
		globaldirectory.entry[i] = oldEntry[2*i];
	}
	delete [] oldEntry;
	return 1;
}