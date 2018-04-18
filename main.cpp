#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

std::vector<int> readfile(const std::string&);


int main(int argc, char** argv) {

	if(argc < 4) {
		std::cout << "Invalid number of arguments.\n";
		return -1;
	}

	std::string image_name(argv[1]);
	int vert_seams = std::atoi(argv[2]);
	int horiz_seams = std::atoi(argv[3]);

	std::vector<int> data = readfile(image_name);

	// The metadata for the image is also contained in the vector
	int xdim = data[0];
	int ydim= data[1];
	int gsmax = data[2];

	//Double-check the size of the image
	if(data.size() - 3 != xdim * ydim) {
		std::stringstream ss;
		ss << "Error - Expected an image of size " << xdim * ydim << ", but got " << data.size() - 3;
		throw std::runtime_error(ss.str());
	}

	int image[ydim][xdim];

	int x = 0, y = 0;
	// Read the data into the image matrix
	for(int i = 3; i < data.size(); ++i) {
		if(data[i] > gsmax) {
			std::stringstream ss;
			ss << "Error - Got value of " << data[i] << ", but max value is " << gsmax;
			throw std::runtime_error(ss.str());
		}

		image[y][x] = data[i];
		++x;

		if(y == ydim) {
			++x;
			y = 0;
		}
	}

	//Print the array for testing
	
	// std::cout << "Image data:\n";
	// for(int i = 0; i < ydim; ++i) {
	// 	for(int j = 0; j < xdim; ++j) {
	// 		printf("%d ", image[i][j]);
	// 	}
	// 	printf("\n");
	// }
	

	// Calculate the energy matrix
	//std::vector<std::vector<int>> energy_matrix(ydim, std::vector<int>(xdim));
	int energy_matrix[ydim][xdim];
	for(int i = 0; i < ydim; ++i) {
		for(int j = 0; j < xdim; ++j) {
			int pixel = image[i][j];
			int energy = 0;
			
			if(i != 0) {
				//Do i-1
				energy += std::abs(pixel - image[i-1][j]);
			}
			if(j != 0) {
				//Do j-1
				energy += std::abs(pixel - image[i][j-1]);
			}
			if(i < ydim - 1) {
				//Do i+1
				energy += std::abs(pixel - image[i+1][j]);
			}
			if(j < xdim - 1) {
				//Do j+1
				energy += std::abs(pixel - image[i][j+1]);
			}

			energy_matrix[i][j] = energy;
		}
	}

	
	// std::cout << "\n\nEnergy matrix:\n";
	// for(int i = 0; i < ydim; ++i) {
	// 	for(int j = 0; j < xdim; ++j) {
	// 		printf("%d ", energy_matrix[i][j]);
	// 	}
	// 	printf("\n");
	// }
	

	//Remove the desired number of vertical seams
	int cumul_energy_vertical[ydim][xdim];
	for(int vseam = 0; vseam < vert_seams; ++vseam) {

		//Calculate the cumulative energy for each pixel
		
		for(int i = 0; i < ydim; ++i) {
		for(int j = 0; j < xdim; ++j) {
			printf("Accessing (%d, %d) = %d\n", i, j, energy_matrix[i][j]);
			int px_energy = energy_matrix[i][j];
			if(i == 0) {
				//The cumulative energy of the top row is the top row of the energy matrix
				cumul_energy_vertical[i][j] = energy_matrix[i][j];
				//printf("\tTop row - ");
			} 
			else if(j == 0) {
				//The cumulative energy of a pixel in the leftmost column
				//the value in the energy matrix plus the pixel straight above plus the one upleft
				int addval = std::min(cumul_energy_vertical[i-1][j], cumul_energy_vertical[i-1][j+1]);

				cumul_energy_vertical[i][j] = px_energy + addval;
				// printf("%d + %d = ", px_energy, addval);
			}
			else if(j == xdim - 1) {
				int addval = std::min(cumul_energy_vertical[i-1][j-1], cumul_energy_vertical[i-1][j]);

				cumul_energy_vertical[i][j] = px_energy + addval;
				// printf("%d + %d = ", px_energy, addval);
			}
			else {
				int addval = std::min({cumul_energy_vertical[i-1][j-1], cumul_energy_vertical[i-1][j], cumul_energy_vertical[i-1][j+1]});
				cumul_energy_vertical[i][j] = px_energy + addval;
				// printf("%d + %d = ", px_energy, addval);
			}
			// printf("%d   ", cumul_energy_vertical[i][j]);
			

		}
		// printf("\n");
		}

	
		std::cout << "\n\nCumulative energy matrix (vertical):\n";
		for(int i = 0; i < ydim; ++i) {
		for(int j = 0; j < xdim; ++j) {
			printf("%d ", cumul_energy_vertical[i][j]);
		}
		printf("\n");
		}
	}

	//Calculate the cumulative horizontal energy
	int cumul_energy_horizontal[ydim][xdim];
	for(int hseam = 0; hseam < horiz_seams; ++horiz_seams) {
		
	}
	

	image_name.insert(image_name.find_first_of("."), "_processed");
}

// Reads all the integer values of a file, skipping non-ints, including metadata
std::vector<int> readfile(const std::string& filename) {
	std::ifstream infile(filename);
	std::vector<int> values;

	
	if(!infile.is_open())
		throw std::runtime_error("Error - Could not open file.");

	std::string line;
	std::getline(infile, line);
	if(line != "P2")
		throw std::runtime_error("Error - Incorrect file format.");

	char c;
	int num = 0;
	bool is_num = false;

	while(infile.get(c)) {
		if(c >= '0' && c <= '9') {
			is_num = true;
			// The difference between a number and its ASCII value is '0'=48
			num = num * 10 + (c - '0');
		}
		else if(is_num) {
			values.push_back(num);
			is_num = false;
			num = 0;
		}
	}

	if(is_num)
		values.push_back(num);

	return values;
}
