// ============================================================================
// Name        : hashskeleton.cpp
// Author      : Faculty of Engineering, Cairo University
// Version     :
// Description : Hashing using Extendible hashing
// ============================================================================

#include <iostream>
#include "ExtendibleHashing.h"

int main() {

    double score = 0.0;

    //Initialize File
    GlobalDirectory globalDirectory;
    Bucket initialFile;

    insertItem(DataItem(220, 220), initialFile, globalDirectory);
    displayDirectory(globalDirectory, initialFile, 0);
    insertItem(DataItem(245, 245), initialFile, globalDirectory);
    displayDirectory(globalDirectory, initialFile, 0);
    insertItem(DataItem(180, 180), initialFile, globalDirectory);
    displayDirectory(globalDirectory, initialFile, 0);
	//============================================================
    insertItem(DataItem(255, 255), initialFile, globalDirectory);
    displayDirectory(globalDirectory, initialFile, 0);
    insertItem(DataItem(1, 1), initialFile, globalDirectory);
    displayDirectory(globalDirectory, initialFile, 0);
	insertItem(DataItem(2, 2), initialFile, globalDirectory);
    displayDirectory(globalDirectory, initialFile, 0);
	//============================================================
	insertItem(DataItem(127, 127), initialFile, globalDirectory);
	displayDirectory(globalDirectory, initialFile, 0);
    searchItem(1, initialFile, globalDirectory);
    searchItem(15, initialFile, globalDirectory);

    return 0;
}