/* On my honor, I have neither given nor received unauthorized aid on this assignment */

/*
Project 2
Dictionary Compression/Decompression
Raz Aloni
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <bitset>
#if defined(_MSC_VER)
#include <unordered_map>
#elif defined(__GNUC__)
#include <tr1/unordered_map>
using namespace std::tr1;
#endif

using namespace std;

typedef unsigned int timestamp;

#define IN_COMPRESS "original.txt"
#define OUT_COMPRESS "cout.txt"
#define IN_DECOMPRESS "compressed.txt"
#define OUT_DECOMPRESS "dout.txt"

void compressData();
string compressLine(bitset<32> rawLine, vector<bitset<32> > dictionary, unsigned int count);
void decompressData();
bool firstIsLarger(pair<string, pair<unsigned int, timestamp> > val1, pair<string, pair<unsigned int, timestamp> > val2)
{
	if (val1.second.first != val2.second.first)
	{
		return val1.second.first > val2.second.first;
	}
	return val1.second.second < val2.second.second;
}

int main(int argc, char * argv[])
{
	/* Check args for compression/decompression argument */

	if (argc != 2)
	{
		/* There is not only 1 argument*/
		cerr << "Invalid number of Arguments [" << argc - 1 << "]. \nPlease provide an argument of \'1\' or \'2\' only.";
		exit(EXIT_FAILURE);
	}

	if (!string(argv[1]).compare("1"))
	{
		/* We are in compression mode */
		compressData();
	}
	else if (!string(argv[1]).compare("2"))
	{
		/* We are in decompression mode */
		decompressData();
	}
	else
	{
		/* Invalid Argument */
		cerr << "Invalid Argument [" << argv[1] << "]. \nPlease provide an argument of \'1\' or \'2\' only.";
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

void compressData()
{
	/* Open File Stream for Input File */
	ifstream input(IN_COMPRESS);

	if (!input.is_open())
	{
		/* Could not open file */
		cerr << "Could not open file \"" << IN_COMPRESS << "\"\nMake sure the file is in the same directory as the executable";
		return;
	}

	/* Count Freqnecy of Lines of Code */
	unordered_map<string, pair<unsigned int, timestamp> > * frequencyCount = new unordered_map<string, pair<unsigned int, timestamp> >();

	timestamp t = 0;

	while (!input.eof())
	{
		string s;
		input >> s;
		
		if (frequencyCount->find(s) == frequencyCount->end())
		{
			/* This line has not been seen before */
			frequencyCount->insert(pair<string, pair<unsigned int, timestamp> >(s, pair<unsigned int, timestamp>(1, t)));
		}
		else
		{
			/* Increment count of this line */
			(*frequencyCount)[s].first++;
		}

		t++;
	}

	vector <pair<string, pair<unsigned int, timestamp> > > * maxVals = new vector<pair<string, pair<unsigned int, timestamp> > >(frequencyCount->begin(), frequencyCount->end());
	stable_sort(maxVals->begin(), maxVals->end(), firstIsLarger);
	

	/* Move 8 most frequent values to dictionary */
	vector<bitset<32> > compressionDictionary(8);

	for (int i = 0; i < 8; i++)
	{
		compressionDictionary[i] = bitset<32>((*maxVals)[i].first);
	}

	/* Delete Unnecessary Data Structures */
	delete frequencyCount;
	delete maxVals;

	/* Return to top of file */
	input.clear();
	input.seekg(0, ios::beg);

	/* Open/create output file */
	ofstream output(OUT_COMPRESS);

	/* Read in one line at a time */
	bitset<32> currentBits, pastBits;
	unsigned int count = 0, cursorPosition = 0;
	string lineIn, compressedLine;

	/* Read In first line into pastBits */ 
	input >> lineIn;
	pastBits = bitset<32>(lineIn);

	while (!input.eof())
	{
		/* Read in next Line */
		input >> lineIn;
		currentBits = bitset<32>(lineIn);

		/* Check if equal to repeating pattern */
		if (pastBits == currentBits && count < 4)
		{
			/* Increase count, do nothing else for now */
			count++;
		}
		else
		{
			/* Process past value */
			compressedLine = compressLine(pastBits, compressionDictionary, count);

			/* Print String to file */
			for (unsigned int i = 0; i < compressedLine.size(); i++)
			{
				if (cursorPosition == 32)
				{
					/* Need to make a new line before putting anymore values */
					output << endl;
					cursorPosition = 0;
				}

				output << compressedLine[i];
				cursorPosition++;
			}

			/* Move current value to past value */
			pastBits = currentBits;

			/* Reset count */
			count = 0;
		}
	}

	/* Process current value */
	compressedLine = compressLine(currentBits, compressionDictionary, count);

	/* Print String to file */
	for (unsigned int i = 0; i < compressedLine.size(); i++)
	{
		if (cursorPosition == 32)
		{
			/* Need to make a new line before putting anymore values */
			output << endl;
			cursorPosition = 0;
		}

		output << compressedLine[i];
		cursorPosition++;
	}

	/* Fill remaining line with 1's*/
	for (; cursorPosition != 32; cursorPosition++)
	{
		output << 1;
	}

	/* Print Out Seperation for Dictionary */
	output << endl << "xxxx";

	/* Print Out Dictionary */
	for (int i = 0; i < 8; i++)
	{
		output << endl << compressionDictionary[i];
	}

	/* Close files */
	input.close();
	output.close();
}

string compressLine(bitset<32> rawLine, vector<bitset<32> > dictionary, unsigned int count)
{
	stringstream result;

	/* Determine Best Compression method to use */

	/* Preference 1: Direct Matching */
	vector<bitset<32> >::iterator it = find(dictionary.begin(), dictionary.end(), rawLine);
	if (it != dictionary.end())
	{
		int index = distance(dictionary.begin(), it);
		result << "101" << bitset<3>(index);
	}
	else
	{
		/* Calculate the mismatches with library entries */
		vector<int> mismatches[8];

		for (int i = 0; i < 8; i++)
		{
			bitset<32> diff = dictionary[i] ^ rawLine;
			
			for (int j = 0; j < 32; j++)
			{
				if (diff.test(j))
				{
					mismatches[i].push_back(j);
				}
			}
		}

		bool compressionFound = false;

		/* Preference 2: 1-Bit Mismatch */
		for (int i = 0; i < 8; i++)
		{
			if (mismatches[i].size() == 1)
			{
				/* Print out 1 bit mismatch */
				result << "010" << bitset<5>(31 - mismatches[i][0]) << bitset<3>(i);
				compressionFound = true;
				break;
			}
		}

		/* Preference 3: Consecutive 2-Bit Mismatch */
		if (!compressionFound)
		{
			for (int i = 0; i < 8; i++)
			{
				if (mismatches[i].size() == 2
					&& mismatches[i][0] + 1 == mismatches[i][1])
				{
					/* Print Out 2 bit mismatch */
					result << "011" << bitset<5>(31 - mismatches[i][1]) << bitset<3>(i);
					compressionFound = true;
					break;
				}
			}
		}

		/* Preference 4: Bitmask Based Compression */
		if (!compressionFound)
		{
			for (int i = 0; i < 8; i++)
			{
				size_t size = mismatches[i].size();

				if (size <= 4 && mismatches[i][size-1] - mismatches[i][0] < 4)
				{
					/* Print Out bitmask mismatch */
					result << "001" << bitset<5>(31-mismatches[i][size - 1]);
					
					/* Calculate bitmask */
					for (int j = mismatches[i][size - 1], k = 1; j > mismatches[i][size - 1] - 4; j--)
					{
						if (k > size || j != mismatches[i][size - k])
						{
							result << 0;
						}
						else
						{
							result << 1;
							k++;
						}
					}

					result << bitset<3>(i);
					compressionFound = true;
					break;
				}
			}
		}

		/* Preference 5: 2-bit Mismatch anywhere */
		if (!compressionFound)
		{
			for (int i = 0; i < 8; i++)
			{
				if (mismatches[i].size() == 2)
				{
					/* Print out 2 bit mismatch */
					result << "100" << bitset<5>(31 - mismatches[i][1]) << bitset<5>(31 - mismatches[i][0]) << bitset<3>(i);
					compressionFound = true;
					break;
				}
			}
		}

		/* Preference 6: Original Binary */
		if (!compressionFound)
		{
			result << "111" << rawLine;
		}
		
	}
	
	

	/* Check if a repeat is needed */
	if (count > 0)
	{
		result << "000" << bitset<2>(count - 1);
	}

	return result.str();
}

void decompressData()
{
	// TODO: All decompression
	/* Open File Stream for Input File */
	ifstream input(IN_DECOMPRESS);

	if (!input.is_open())
	{
		/* Could not open file */
		cerr << "Could not open file \"" << IN_DECOMPRESS << "\"\nMake sure the file is in the same directory as the executable";
		return;
	}

	/* Get dictionary */
	string s;
	do
	{
		getline(input, s);
	} 
	while (s != "xxxx");

	vector<bitset<32>> compressionDictionary;

	while (!input.eof())
	{
		getline(input, s);
		compressionDictionary.push_back(bitset<32>(s));
	}

	/* Return to top of file */
	input.clear();
	input.seekg(0, ios::beg);

	ofstream output(OUT_DECOMPRESS);

	bitset<32> previousOutput = 0;

	bitset<2> count;
	bitset<3> compressionType, dictionaryIndex;
	bitset<4> bitMask;
	bitset<5> firstLocation, secondLocation;

	while (!input.eof())
	{
		/* Decompress Line by line */
		/* Read in 3 characters */
		input >> compressionType;

		switch (compressionType.to_ulong())
		{
			case 0: /* Run Length Encoding, repeat previous value */
				input >> count;
				for (int i = 0; i < count.to_ulong() + 1; i++)
				{
					output << previousOutput << endl;
				}
				
				break;
			case 1: /* Bit mask based compression, read in dictionary value and flip necessary bits */
				input >> firstLocation >> bitMask >> dictionaryIndex;
				// TODO: Complete Bitmask
				break;
			case 2: /* 1 bit Mismatch */
				input >> firstLocation >> dictionaryIndex;
				// TODO: Complete One bit
				break;
			case 3: /* 2-bit consecutive mismatch */
				input >> firstLocation >> dictionaryIndex;
				// TODO: Complete 2 bit
				break;
			case 4: /* 2-bit mismatch anywhere */
				input >> firstLocation >> secondLocation >> dictionaryIndex;
				// TODO: Complete 2 bit anywhere
				break;
			case 5: /* Direct Matching */
				input >> dictionaryIndex;
				previousOutput = compressionDictionary[dictionaryIndex.to_ulong()];
				output << previousOutput;
				break;
			case 7: /* Original Binary */
				input >> previousOutput;
				output << previousOutput;
		}
	}

	//TODO: Account for reverse bits?

	output.close();
}
