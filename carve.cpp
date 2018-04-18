#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cstdio>

typedef std::vector<int> Row;
typedef std::vector<Row> Matrix;

std::vector<int> readfile(const std::string&);
void fillImage(const std::vector<int>&, Matrix&);
void removeVerticalSeam(Matrix&, int, int);
void removeHorizontalSeam(Matrix&, int, int);
Matrix energyMatrix(const Matrix&, int, int);
Matrix cumulativeEnergyMatrixVertical(const Matrix&, int, int);
Matrix cumulativeEnergyMatrixHorizontal(const Matrix&, int, int);
void printMatrix(const Matrix&, int, int);

int main(int argc, char** argv) {

	if(argc < 4) {
		std::cout << "Invalid number of arguments.\n";
		return -1;
	}

	std::string filename(argv[1]);
	int vert_seams = std::atoi(argv[2]);
	int horiz_seams = std::atoi(argv[3]);

	std::vector<int> data = readfile(filename);

	int xdim = data[0];
	int ydim = data[1];
	
	//Double-check the size of the image 
	if(data.size() - 3 != xdim * ydim) {
		std::stringstream ss;
		ss << "Error - expected an image of size " << xdim * ydim << ", but got " << data.size() - 3;
		throw std::runtime_error(ss.str());
	}

	//Allocate storage for the image
	Matrix image(ydim, Row(xdim));

	fillImage(data, image);

	std::cout << "Image data:\n";
	printMatrix(image, xdim, ydim);


	std::cout << "\n\nEnergy matrix (vertical):\n";
	Matrix energy = energyMatrix(image, xdim, ydim);
	printMatrix(energy, xdim, ydim);


	std::cout << "\n\nCumulative energy matrix (vertical):\n";
	Matrix cverten = cumulativeEnergyMatrixVertical(energy, xdim, ydim);
	printMatrix(cverten, xdim, ydim);


	std::cout << "\n\nCumulative energy matrix (horizontal):\n";
	Matrix cHorizEn = cumulativeEnergyMatrixHorizontal(energy, xdim, ydim);
	printMatrix(cHorizEn, xdim, ydim);

	/*
	for(int i = 0; i < vert_seams; ++i) {
		removeVerticalSeam(image, xdim, ydim);
	}

	for(int i = 0; i < horiz_seams; ++i) {
		removeHorizontalSeam(image, xdim, ydim);
	}
	*/


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

void fillImage(const std::vector<int>& data, Matrix& image) {
	int x = 0;
	int y = 0;
	int xdim = data[0];
	int gsmax = data[2];

	for(int i = 3; i < data.size(); ++i) {
		if(data[i] > gsmax) {
			std::stringstream ss;
			ss << "Error - got value of " << data[i] << ", but max value is " << gsmax;
			throw std::runtime_error(ss.str());
		}
		if(x == xdim) {
			x = 0;
			++y;
		}
		image[y][x] = data[i];
		++x;
	}	
}

void removeVerticalSeam(Matrix& image, int xdim, int ydim) {
	
}

void removeHorizontalSeam(Matrix& image, int xdim, int ydim) {

}

Matrix energyMatrix(const Matrix& image, int xdim, int ydim) {
	Matrix energy_matrix(ydim, Row(xdim));

	for(int y = 0; y < ydim; ++y) {
		for(int x = 0; x < xdim; ++x) {
			int pixel = image[y][x];
			int energy = 0;

			if(y != 0) {
				energy += std::abs(pixel - image[y-1][x]);
			}
			if(x != 0) {
				energy += std::abs(pixel - image[y][x-1]);
			}
			if(y < ydim - 1) {
				energy += std::abs(pixel - image[y+1][x]);
			}
			if(x < xdim - 1) {
				energy += std::abs(pixel - image[y][x+1]);
			}

			energy_matrix[y][x] = energy;
		}
	}

	return energy_matrix;
}


Matrix cumulativeEnergyMatrixVertical(const Matrix& energy_matrix, int xdim, int ydim) {
	Matrix cumul_energy(ydim, Row(xdim));

	for(int y = 0; y < ydim; ++y) {
		for(int x = 0; x < xdim; ++x) {
			int px_energy = energy_matrix[y][x];
			if(y == 0) {
				//The cumulative energy of the top row is the top row of the energy matrix
				cumul_energy[y][x] = energy_matrix[y][x];
			} 
			else if(x == 0) {
				//The cumulative energy of a pixel in the leftmost column
				//the value in the energy matrix plus the pixel straight above plus the one upleft
				int addval = std::min(cumul_energy[y-1][x], cumul_energy[y-1][x+1]);
				cumul_energy[y][x] = px_energy + addval;
			}
			else if(x == xdim - 1) {
				int addval = std::min(cumul_energy[y-1][x-1], cumul_energy[y-1][x]);
				cumul_energy[y][x] = px_energy + addval;
			}
			else {
				int addval = std::min({cumul_energy[y-1][x-1], cumul_energy[y-1][x], cumul_energy[y-1][x+1]});
				cumul_energy[y][x] = px_energy + addval;
			}
		}
	}
	return cumul_energy;
}


Matrix cumulativeEnergyMatrixHorizontal(const Matrix& energy_matrix, int xdim, int ydim) {
	Matrix cumul_energy(ydim, Row(xdim));

	for(int x = 0; x < xdim; ++x) {
		for(int y = 0; y < ydim; ++y) {
			int px_energy = energy_matrix[y][x];
			if(x == 0) {
				//The cumulative energy of the left row is the left row of the energy matrix
				cumul_energy[y][x] = px_energy;
			} 
			else if(y == 0) {
				//The cumulative energy of the top row is the min of y,x-1 and y+1,x-1
				int addval = std::min(cumul_energy[y][x-1], cumul_energy[y+1][x-1]);
				cumul_energy[y][x] = px_energy + addval;
			}
			else if(y == ydim - 1) {
				//The cumulative energy of the bottom row is the min of y,x-1 and y-1,x-1
				int addval = std::min(cumul_energy[y][x-1], cumul_energy[y-1][x-1]);
				cumul_energy[y][x] = px_energy + addval;
			}
			else {
				int addval = std::min({cumul_energy[y-1][x-1], cumul_energy[y][x-1], cumul_energy[y+1][x-1]});
				cumul_energy[y][x] = px_energy + addval;
			}
		}
	}
	return cumul_energy;
}

void printMatrix(const Matrix& matrix, int xdim, int ydim) {
	for(int y = 0; y < ydim; ++y) {
		for(int x = 0; x < xdim; ++x) {
			std::cout << matrix[y][x] << ' ';
		}
		std::cout << '\n';
	}
}