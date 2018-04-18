#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <gli/gli.hpp>

using namespace std;

#include "CImg.h"
using namespace cimg_library;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void filter(CImg<float>& imageInput, CImg<float>& imageOutput, const int level, const int Nlevels)
{
    // distance to texture plane
    // in shader: LOD = log(powf(2.0f, Nlevels - 1.0f) * dist) / log(3)
    const float dist = powf(3.0f, level) / powf(2.0f, Nlevels - 1.0f);

    // filter size
    // at distance 1 ~= Gaussian of std 0.75
    const float filterStd = 0.75f * dist * imageInput.width();

    cout << "level " << level << endl;
    cout << "distance to texture plane = " << dist << endl;
    cout << "filterStd = " << filterStd << endl << endl;

    CImg<float> tmp(imageInput.width(), imageInput.height(), 1, 4);

    for (int j = 0; j < imageInput.height(); ++j)
    for (int i = 0; i < imageInput.width();  ++i)
    {
        tmp(i, j, 0, 0) = imageInput(i, j, 0, 0);
        tmp(i, j, 0, 1) = imageInput(i, j, 0, 1);
        tmp(i, j, 0, 2) = imageInput(i, j, 0, 2);
        tmp(i, j, 0, 3) = 1.0f;
    }

    tmp.blur(filterStd, filterStd, filterStd, false);

    // renormalise based on alpha
    for (int j = 0; j < imageInput.height(); ++j)
    for (int i = 0; i < imageInput.width();  ++i)
    {
        float alpha = tmp(i, j, 0, 3);
        for (int k = 0; k < tmp.spectrum(); ++k)
          tmp(i, j, 0, k) /= alpha;
    }

    // rescale image
    imageOutput = tmp.resize(imageOutput, 5); // 5 = cubic interpolation
    return;
}

int main(int argc, char* argv[])
{

    // Skip executable argument
    argc--;
    argv++;

    if (argc < 1)
    {
        printf("Syntax: <input file>\n");
        return -1;
    }

    string filenameInput(argv[0]);

	size_t pos = filenameInput.find_last_of(".");
    string filename  = filenameInput.substr(0, pos);
    string extension = filenameInput.substr(pos + 1, string::npos);

    // input image
    int x, y, n;
    float* data = stbi_loadf(filenameInput.c_str(), &x, &y, &n, 3);
    if (data == nullptr)
    {
        cerr << "can't find file " << filenameInput.c_str() << endl;
        return -1;
    }

    int offset = 0;
    CImg<float> imageInput(x, y, 1, 3);
    for (int j = 0; j < y; ++j)
    for (int i = 0; i < x; ++i)
    {
        for (int k = 0; k < imageInput.spectrum(); ++k)
            imageInput(i, j, 0, k) = data[offset++];
    }

	// linearize and save to dds
	{
		gli::extent3d extent(imageInput.width(), imageInput.height(), 1); 
		gli::texture texture(gli::TARGET_2D_ARRAY, gli::FORMAT_RGBA32_SFLOAT_PACK32, extent, 1, 1, 1);

        offset = 0;
        float* dest = reinterpret_cast<float*>(texture.data(0, 0, 0));
        for (int j = 0; j < y; ++j)
        for (int i = 0; i < x; ++i)
        {
            dest[offset++] = imageInput(i, j, 0, 0);
            dest[offset++] = imageInput(i, j, 0, 1);
            dest[offset++] = imageInput(i, j, 0, 2);
            dest[offset++] = 1.f;
        }
		stringstream filenameOutput (stringstream::in | stringstream::out);
		filenameOutput << filename << ".dds"; 
		gli::save(texture, filenameOutput.str());
	}

	imageInput.resize(512, 512, 1, 3);

    // filtered levels
    //unsigned int Nlevels;
    //for (Nlevels = 1; (imageInput.width() >> Nlevels) > 0; ++Nlevels);

    // Beyond 8 levels, the result is ~= a constant colour
    // so pretend we have 12 levels, but truncate at 8
    const unsigned int Nlevels = 12;
    const unsigned int maxLevels = 8;
	
    size_t levels = static_cast<size_t>(Nlevels);
    gli::extent3d extent(imageInput.width(), imageInput.height(), 1); 
    gli::texture texture(gli::TARGET_2D_ARRAY, gli::FORMAT_RGBA32_SFLOAT_PACK32, extent, maxLevels, 1, 1);

    // borders
    stringstream filenameOutput (stringstream::in | stringstream::out);
    filenameOutput << filename << "_filtered" << ".dds"; 
    for (unsigned int idx = 0; idx < maxLevels; ++idx)
    {
        cout << "processing file " << filenameOutput.str() << " Level: " << idx << endl;
        CImg<float> imageOutput(x, y, 1, 4);
        filter(imageInput, imageOutput, idx, Nlevels);

        offset = 0;
        float* dest = reinterpret_cast<float*>(texture.data(idx, 0, 0));
        for (int j = 0; j < y; ++j)
        for (int i = 0; i < x; ++i)
        {
            dest[offset++] = imageOutput(i, j, 0, 0);
            dest[offset++] = imageOutput(i, j, 0, 1);
            dest[offset++] = imageOutput(i, j, 0, 2);
            dest[offset++] = 1.f;
        }
    }
    gli::save(texture, filenameOutput.str());

    return 0;
}
