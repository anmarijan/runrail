# Runrail

This program is a simple train simulation to make a train diagram (speed-distance), written in C++.

## Functions of the current version
- Simulate a train operation along a line and make a train diagram (speed-distance).
- Save the result diagram as a SVG file

## Status
This program is in development stage.

## Install and Use
- Install: Compile *.cpp files to make the exe file. Needs [nlohmann/json.hpp](https://github.com/nlohmann/json).
- Usage: runrail input_name output_name svg_name
## Input data
There are two input data. Sample files are located in data folder.
- Parameter file in json format: All input data except for the line data. It includes train parameters, speed-traction relationship, and the file name of the line file.
- Line file: The line data is not included in the parameter file because editing of line data in json format is not convinient.

## Output data
- Calculated data of the location, speed, time, status, and power.

## Parameter format
### line
This is an array of lines. Each line has "id", "type", and "fname" objects.
### speedtraction
This is an array of speed-traction relationships. Parameters are: "id", "name", "unit", and "data".
### train
This is an array of trains.