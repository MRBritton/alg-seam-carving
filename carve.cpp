#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <utility>
#include <typeinfo>

typedef std::vector<int> Row;
typedef std::vector<Row> Matrix;
typedef std::pair<int, int> Pixel;

std::vector<int> readfile(const std::string&);
void fillImage(const std::vector<int>&, Matrix&);

void removeVerticalSeam(Matrix&, int&, int&);
void removeHorizontalSeam(Matrix&, int&, int&);

Matrix energyMatrix(const Matrix&, int, int);
Matrix cumulativeEnergyMatrixVertical(const Matrix&, int, int);
Matrix cumulativeEnergyMatrixHorizontal(const Matrix&, int, int);

Row::iterator minColumnElement(Matrix&, int, int, int);

template<typename Iter>
Iter minPointedValues(std::vector<Iter>&);

template<typename Iter>
bool isInRange(Iter, Iter, Iter);

template<typename Iter>
int findYIndexInMatrix(Iter i, Matrix& m, int xdim, int ydim);

void printMatrix(const Matrix&, int, int);
void writeNewImage(const std::string&, const Matrix&, int, int, int);


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
	int gsmax = data[2];

	
	//Double-check the size of the image 
	if(data.size() - 3 != xdim * ydim) {
		std::cout << "Error - expected an image of size " << xdim * ydim << ", but got " << data.size() - 3 << '\n';
		return -1;
	}

	//Make sure that there is actually image data to process
	if(data.size() - 3 <= 0) {
		std::cout << "Error - cannot process an image of size 0.\n";
		return -1;
	}

	//Allocate storage for the image
	Matrix image(ydim, Row(xdim));

	fillImage(data, image);

	for(int i = 0; i < vert_seams; ++i) {
		removeVerticalSeam(image, xdim, ydim);
	}

	for(int i = 0; i < horiz_seams; ++i) {
		removeHorizontalSeam(image, xdim, ydim);
	}

	filename.insert(filename.find_first_of("."), "_processed");
	writeNewImage(filename, image, xdim, ydim, gsmax);
	std::cout << "Processed file written to " << filename << '\n';
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

// Fill the image matrix with the values read from the file
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

// Remove one vertical seam from the image
void removeVerticalSeam(Matrix& image, int& xdim, int& ydim) {
	Matrix energy_matrix = energyMatrix(image, xdim, ydim);
	Matrix vertical_energy = cumulativeEnergyMatrixVertical(energy_matrix, xdim, ydim);

	//Pixels marked to be removed
	std::vector<Pixel> pixels_to_remove;

	//Work up from the bottom row
	int y = ydim - 1;
	Row& currentRow = vertical_energy[y];
	auto min = std::min_element(currentRow.begin(), currentRow.end());
	int x = std::distance(currentRow.begin(), min); // xcoord of bottom min - start of solution

	while(y >= 0) {		
		pixels_to_remove.push_back({y, x});
		
		--y;
		if(y < 0)
			break;

		currentRow = vertical_energy[y];

		//find the x index of the minimum value of the 2 to 3 pixels above the current pixel
		x = std::distance(currentRow.begin(), 
			std::min_element(currentRow.begin() + (x - 1 > -1 ? x - 1 : x), 
				currentRow.begin() + (x + 1 < xdim ? x + 2 : x + 1)
				)
			);
	}

	for(auto i : pixels_to_remove) {
		image[i.first].erase(image[i.first].begin() + i.second);
	}
	--xdim;
}


// Remove one horizontal seam from the image
void removeHorizontalSeam(Matrix& image, int& xdim, int& ydim) {
	Matrix energy_matrix = energyMatrix(image, xdim, ydim);
	Matrix horizontal_energy = cumulativeEnergyMatrixHorizontal(energy_matrix, xdim, ydim);

	//Start backtracing from the right
	int x = xdim - 1;
	auto min_in_range = minColumnElement(horizontal_energy, xdim, ydim, x);

	int y = findYIndexInMatrix(min_in_range, horizontal_energy, xdim, ydim);
	
	std::vector<Pixel> pixels_to_remove;

	pixels_to_remove.push_back({y, x});

	while(x > 0) {
		//Look at the next column
		--x;

		std::vector<Row::iterator> pixels_to_check;

		if(y == 0) {
			// Look at (y,x), (y+1,x)
			pixels_to_check.push_back(horizontal_energy[y].begin() + x);

			if(y + 1 != ydim)
				pixels_to_check.push_back(horizontal_energy[y+1].begin() + x);
		}
		else if(y == ydim - 1) {
			// Look at y-1, y
			pixels_to_check.push_back(horizontal_energy[y-1].begin() + x);
			pixels_to_check.push_back(horizontal_energy[y].begin() + x);
		}
		else {
			// Look at y-1, y, y+1
			pixels_to_check.push_back(horizontal_energy[y-1].begin() + x);
			pixels_to_check.push_back(horizontal_energy[y].begin() + x);
			pixels_to_check.push_back(horizontal_energy[y+1].begin() + x);
		}

		//Find minimum of pointed pixels
		min_in_range = minPointedValues(pixels_to_check);

		//update y
		y = findYIndexInMatrix(min_in_range, horizontal_energy, xdim, ydim);

		pixels_to_remove.push_back({y,x});
	}

	//Can't just call vector::erase since these could be in any one of the rows
	//Remove pixels marked for deletion
	for(auto i : pixels_to_remove) {
		image[i.first][i.second] = -1;
	}

	//Overwrite any deleted pixels by shifting up all values below them
	for(int ydx = 0; ydx < ydim; ++ydx) {
		for(int xdx = 0; xdx < xdim; ++xdx) {
			if(image[ydx][xdx] == -1) {
				for(int k = ydx; k < ydim - 1; ++k) {
					image[k][xdx] = image[k + 1][xdx];
				}
			}
		}
	}

	//Delete the bottom row
	image.erase(image.begin() + ydim - 1);

	//Make note of the image's reduced y-dimension
	--ydim;
}

//Min element of the given column
Row::iterator minColumnElement(Matrix& m, int xdim, int ydim, int col) {
	if(col >= xdim) {
		throw std::runtime_error("Error - attempted to read column out-of-bounds.");
	}

	std::vector<int> col_values;

	for(int y = 0; y < ydim; ++y) {
		col_values.push_back(m[y][col]);
	}

	//find the minimum of the values in the column
	auto min_col = std::min_element(col_values.begin(), col_values.end());

	for(int y = 0; y < ydim; ++y) {
		if(m[y][col] == *min_col) {
			return m[y].begin() + col;
		}
	}

	//if we get down here there's been some kind of error, every column should have a minimum
	throw std::logic_error("Something has gone terribly wrong.");

	return {};
}

//Find the y-index of the value at a given iterator in the matrix
template<typename Iter>
int findYIndexInMatrix(Iter i,Matrix& m, int xdim, int ydim) {
	for(int y = 0; y < ydim; ++y) {
		if(isInRange(i, m[y].begin(), m[y].end())){
			return y;
		}
	}
	return -1;
}

//Determine whether a given iterator is in the given range
template<typename Iter>
bool isInRange(Iter i, Iter begin, Iter end) {
	return begin <= i && i < end;
}

//Determine which iterator of the vector points to the smallest value
template<typename Iter>
Iter minPointedValues(std::vector<Iter>& ptrs) {
	auto min = *ptrs[0];
	Iter ptr_to_min = ptrs[0];

	for(int i = 0; i < ptrs.size(); ++i) {
		if(*ptrs[i] < min) {
			min = *ptrs[i];
			ptr_to_min = ptrs[i];
		}
	}

	return ptr_to_min;
}

// Calculate the energy matrix of the given image
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

// Calculate the cumulative vertical energy matrix from the given energy matrix
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

// Calculate the cumulative horizontal energy matrix from the given energy matrix
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

// Print the given matrix of given dimensions
void printMatrix(const Matrix& matrix, int xdim, int ydim) {
	for(int y = 0; y < ydim; ++y) {
		for(int x = 0; x < xdim; ++x) {
			std::cout << matrix[y][x] << ' ';
		}
		std::cout << '\n';
	}
}

// Print the modified image to the given filename 
void writeNewImage(const std::string& filename, const Matrix& image, int xdim, int ydim, int gsmax) {
	std::ofstream outfile(filename);
	if(!outfile.is_open()) {
		std::stringstream ss;
		ss << "Error - could not open file " << filename << " to write processed image.";
		throw std::runtime_error(ss.str());
	}

	outfile << "P2\n";
	outfile << "# Processed image\n";
	outfile << xdim << ' ' << ydim << '\n';
	outfile << gsmax << '\n';

	for(int y = 0; y < ydim; ++y) {
		for(int x = 0; x < xdim; ++x) {
			outfile << image[y][x] << ' ';
		}
		outfile << '\n';
	}
}