# Flight Trajectory Visualizer

A native Windows C++ project that trains a small neural network on flight samples, simulates a predicted trajectory, and renders the result in an interactive Direct2D viewer.

## Features

- Reads flight telemetry from CSV input.
- Builds normalized training and target matrices for a feedforward neural network.
- Trains a fully connected neural network in C++.
- Simulates a predicted path from a seed sample until landing or stop conditions.
- Opens a native Windows window using Win32.
- Renders an interactive pseudo-3D trajectory view using Direct2D.
- Displays an altitude profile and telemetry panel.
- Supports mouse orbit, mouse wheel zoom, and playback animation.
- Colors the path by altitude for easier visual inspection.

## Project structure

```text
project/
├── data/
│   └── flight_sample.csv
├── src/
│   ├── NNM.cpp
│   ├── CSVReader.hpp
│   ├── CSVReader.cpp
│   ├── NeuralNetwork.hpp
│   ├── NeuralNetwork.cpp
│   ├── Simulator.hpp
│   ├── Simulator.cpp
│   ├── FlightWindow.hpp
│   └── FlightWindow.cpp
└── README.md
```

## Requirements

- Windows 10 or newer.
- Visual Studio 2019 or newer, or MSVC Build Tools with Windows SDK.
- C++17 support.
- Direct2D and DirectWrite libraries from the Windows SDK.

## Build notes

Link these libraries:

```cpp
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
```

If you use CMake, make sure the target links against `d2d1` and `dwrite` and is built as a Windows desktop application.

## Data format

The CSV is expected to contain flight samples with fields similar to:

```text
timeSec,latitude,longitude,altitude,pitchDeg,yawDeg,speed,...
```

The exact column order should match the implementation inside `CSVReader::readFlightCSV()`.

## How it works

### 1. Load samples

`CSVReader` reads historical flight samples from `data/flight_sample.csv`.

### 2. Build model inputs

`Simulator::buildInputMatrix(samples)` creates the neural-network input matrix.

`Simulator::buildTargetMatrix(samples)` creates the prediction targets, typically representing the next-step delta or next-step state.

### 3. Normalize data

The project computes normalization statistics for both inputs and targets:

- `NeuralNetwork::fitNormalization(...)`
- `NeuralNetwork::normalize(...)`

This keeps training numerically stable and makes features with different scales easier to learn.

### 4. Train the model

A feedforward neural network is created and trained:

```cpp
NeuralNetwork nn(9, 32, 24, 6, 0.12);
nn.train(Xnorm, Ynorm, 7000);
```

This means:

- 9 input features
- 2 hidden layers with 32 and 24 neurons
- 6 output values
- learning rate 0.12
- 7000 epochs

## Simulation flow

After training:

1. A seed sample is chosen from the dataset.
2. The simulator repeatedly predicts the next state.
3. The predicted state is appended to the output path.
4. The loop stops when landing or another stop condition is reached.

Example:

```cpp
auto predictedPath = Simulator::simulateUntilLanding(
    nn,
    seed,
    inputStats,
    targetStats,
    400,
    1.0
);
```

## Visualization

The viewer is implemented with Win32 + Direct2D + DirectWrite.

### Panels

- **3D flight scene**: pseudo-3D perspective rendering of the trajectory above a ground grid.
- **Altitude profile**: a 2D chart showing altitude over simulated time.
- **Telemetry panel**: current sample values, camera state, and controls.

### Interaction

- **Left mouse drag**: orbit camera.
- **Mouse wheel**: zoom in and out.
- **Space**: pause or resume animation.
- **R**: reset camera and replay animation.

### Rendering approach

The 3D view is not full Direct3D. Instead, world points are projected manually into screen space and then drawn using Direct2D primitives like lines, rectangles, ellipses, and text.

## Example main flow

```cpp
auto samples = CSVReader::readFlightCSV("data/flight_sample.csv");
auto Xraw = Simulator::buildInputMatrix(samples);
auto Yraw = Simulator::buildTargetMatrix(samples);

auto inputStats = NeuralNetwork::fitNormalization(Xraw);
auto targetStats = NeuralNetwork::fitNormalization(Yraw);

std::vector<std::vector<double>> Xnorm, Ynorm;
for (const auto& row : Xraw) Xnorm.push_back(NeuralNetwork::normalize(row, inputStats));
for (const auto& row : Yraw) Ynorm.push_back(NeuralNetwork::normalize(row, targetStats));

NeuralNetwork nn(9, 32, 24, 6, 0.12);
nn.train(Xnorm, Ynorm, 7000);

FlightSample seed = samples[samples.size() / 2];
auto predictedPath = Simulator::simulateUntilLanding(nn, seed, inputStats, targetStats, 400, 1.0);

FlightWindow::Show(predictedPath);
```

## Troubleshooting

### Window opens but nothing is drawn

- Confirm `predictedPath` is not empty.
- Check that Direct2D device resources were created successfully.
- Make sure `WM_PAINT` is reaching the render path.

### Build errors for Direct2D or DirectWrite

- Verify Windows SDK is installed.
- Verify `#include <d2d1.h>` and `#include <dwrite.h>` resolve correctly.
- Verify the project links `d2d1.lib` and `dwrite.lib`.

### Strange projection or clipping

- Adjust `m_cameraDistance`, `m_focalLength`, `m_yaw`, `m_pitch`, or world scaling values.
- Large latitude and longitude ranges may need different scale factors.

### Animation too slow or too fast

- Adjust the timer interval in `SetTimer(m_hwnd, 1, 33, nullptr);`.
- Adjust how many samples become visible on each timer tick.

## Possible next upgrades

- Add panning with right mouse drag.
- Add keyboard camera movement.
- Add a scrubber/timeline for the simulation.
- Export trajectory screenshots.
- Add CSV validation and better error messages.
- Replace pseudo-3D projection with a true Direct3D renderer.

## Safety note

This project should be used as a generic flight-data visualization and prediction demo. It is best suited for learning, simulation, UI work, and experimentation with time-series prediction and native Windows graphics.
