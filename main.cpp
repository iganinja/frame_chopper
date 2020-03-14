#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include "lodepng.h"

using namespace std;

using Pixel = std::uint32_t; // RGBA
constexpr unsigned PixelSize = sizeof(Pixel);
using ImageData = std::vector<unsigned char>; // We assume it is in RGBA format
using ImageIt = ImageData::const_iterator;
using Frame = ImageData;


tuple<ImageData, unsigned, unsigned> loadFile(const string& fileName)
{
    ImageData image;
    unsigned imageWidth, imageHeight;

    cout << "Loading " << fileName << " file\n";

    const auto error = lodepng::decode(image, imageWidth, imageHeight, fileName);
    if(error)
    {
        stringstream errorMessage;
        errorMessage << "Cannot open " << fileName << " file (" << error << "): " << lodepng_error_text(error) << std::endl;
        throw runtime_error{errorMessage.str()};
    }

    return make_tuple(image, imageWidth, imageHeight);
}

template<typename Input, typename Output>
void copyPixelBlock(Input inputIt, Output outputIt, int inputStep, int outputStep, int sizeToCopy, int lines)
{
    auto iIt = inputIt;
    auto oIt = outputIt;

    for(int i = 0; i < lines; ++ i)
    {
        copy(iIt, iIt + sizeToCopy, oIt);
        iIt += inputStep;
        oIt += outputStep;
    }
}

Frame getFrame(ImageData::const_iterator frameIt, int framePixelLineSize, int filePixelLineSize, int frameHeight)
{
    Frame frame(framePixelLineSize * frameHeight);

    copyPixelBlock(frameIt, frame.begin(), filePixelLineSize, framePixelLineSize, framePixelLineSize, frameHeight);

    return frame;
}

void addFrame(const Frame& frame, ImageData::iterator imageIt, int framePixelLineSize, int filePixelLineSize, int frameHeight)
{
    copyPixelBlock(frame.begin(), imageIt, framePixelLineSize, filePixelLineSize, framePixelLineSize, frameHeight);
}

void saveFile(const string& outputFileName, int maximumHorizontalFrames, int frameWidth, int frameHeight, const std::vector<Frame>& frames)
{
    const auto horizontalFrames = min(maximumHorizontalFrames, static_cast<int>(frames.size()));
    const auto fileWidth = horizontalFrames * frameWidth;
    const auto fileHeight = ceil(frames.size() / static_cast<float>(horizontalFrames)) * frameHeight;

    ImageData image;
    image.resize(fileWidth * fileHeight * PixelSize);

    auto x = 0;
    ImageData::iterator imageIt = image.begin();

    const auto horizotalStep = frameWidth * PixelSize;
    const auto verticalStep = fileWidth * frameHeight * PixelSize;

    for(const auto& frame : frames)
    {
        addFrame(frame, imageIt, frameWidth * PixelSize, fileWidth * PixelSize, frameHeight);

        ++ x;

        if(x >= horizontalFrames)
        {
            x = 0;
            imageIt += verticalStep - horizotalStep * (horizontalFrames - 1); // Go to the next row and to the left most frame
        }
        else
        {
            imageIt += horizotalStep;
        }
    }

    cout << "Saving " << outputFileName << " file: " << fileWidth << "x" << fileHeight << ", " << frames.size() << " frames in total\n";

    const auto error = lodepng::encode(outputFileName, image.data(), fileWidth, fileHeight);

    if(error)
    {
        std::cout << "Cannot save " << outputFileName << " file (" << error << "): " << lodepng_error_text(error) << std::endl;
    }
}

int main(int argumentNumber, char* arguments[])
{
    if(argumentNumber != 7)
    {
        cout << "frame_chopper 1.0\n";
        cout << "Usage: frame_chopper file_with_frames.png horizontal_frame_number vertical_frame_number output_file.png max_horizontal_frame_number frame_counter_step\n";
        cout << "Example of chopping every odd frame: frame_chopper big.png 10 10 not_so_big.png 10 1\n";
        return 0;
    }

    try
    {
        const string inputFileName = arguments[1];
        const auto inputHorizontalFrames = stoi(arguments[2]);
        const auto inputVerticalFrames = stoi(arguments[3]);
        const auto outputFileName = arguments[4];
        const auto outputMaxHorizontalFrames = stoi(arguments[5]);
        const auto frameCounterStep = stoi(arguments[6]);

        const auto frameNumber = inputHorizontalFrames * inputVerticalFrames;

        const auto fileData = loadFile(inputFileName);
        const auto fileWidth = get<1>(fileData);
        const auto fileHeight = get<2>(fileData);
        const auto frameWidth = fileWidth / inputHorizontalFrames;
        const auto frameHeight = fileHeight / inputVerticalFrames;


        cout << "Loaded successfully: " << fileWidth << "x" << fileHeight << " size with " << frameNumber << " frames of " << frameWidth << "x" << frameHeight << endl;

        std::vector<Frame> mFramesToSave;
        mFramesToSave.reserve(frameNumber);

        const auto& imageData = get<0>(fileData);

        const auto framePixelLineSize = frameWidth * PixelSize;
        const auto frameSize = framePixelLineSize * frameHeight;
        const auto fileFramesLineSize = frameSize * inputHorizontalFrames;
        const auto filePixelLineSize = fileWidth * PixelSize;

        for(int frameIndex = 0; frameIndex < frameNumber; frameIndex += frameCounterStep)
        {
            int x = frameIndex % inputHorizontalFrames;
            int y = frameIndex / inputHorizontalFrames;
            const auto frameIt = imageData.begin() + y * fileFramesLineSize + x * framePixelLineSize;
            mFramesToSave.push_back(getFrame(frameIt, framePixelLineSize, filePixelLineSize, frameHeight));
        }

        saveFile(outputFileName, outputMaxHorizontalFrames, frameWidth, frameHeight, mFramesToSave);
    }
    catch(const exception& e)
    {
        cout << "Error: " << e.what() << endl;
        return -1;
    }

    return 0;
}
