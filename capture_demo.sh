#!/bin/bash

set -e

echo "Building capture program..."
make capture

echo "Creating frames directory..."
mkdir -p frames

echo "Capturing 30 seconds of peaceful waves..."
./capture --capture

echo "Converting frames to animated GIF..."
if command -v ffmpeg &> /dev/null; then
    ffmpeg -framerate 30 -pattern_type glob -i 'frames/frame_*.ppm' \
           -vf "scale=800:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
           -loop 0 peaceful_waves.gif

    echo "Extracting a single representative frame as PNG..."
    ffmpeg -i frames/frame_00450.ppm peaceful_snapshot.png

    echo "Creating optimized version for README..."
    ffmpeg -i peaceful_waves.gif -vf "scale=400:-1" -r 15 peaceful_waves_small.gif

elif command -v convert &> /dev/null; then
    echo "Using ImageMagick..."
    convert -delay 3 -loop 0 frames/frame_*.ppm peaceful_waves.gif
    convert frames/frame_00450.ppm peaceful_snapshot.png
    convert peaceful_waves.gif -resize 400x peaceful_waves_small.gif
else
    echo "Neither ffmpeg nor ImageMagick found. Installing one of them is required."
    echo "For best results: sudo apt-get install ffmpeg"
    echo "Alternative: sudo apt-get install imagemagick"
    exit 1
fi

echo "Cleaning up frames..."
rm -rf frames

echo "Done! Created:"
echo "  - peaceful_snapshot.png (single frame for README)"
echo "  - peaceful_waves.gif (full animation)"
echo "  - peaceful_waves_small.gif (smaller version for README)"
echo ""
echo "To add to README, use:"
echo "  ![Peaceful Waves](peaceful_snapshot.png)"
echo "  or for animated version:"
echo "  ![Peaceful Waves Animation](peaceful_waves_small.gif)"