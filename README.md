# Analytics Modules

## VP - Vision Processing

This module is a generic framework to run data processing in a pipeline
defined as a graph.
The initial target is for vision processing.

## Vision

Compute Vision module.

### Face Detection

Detects faces from the image.

Usage:

```
vision --mq-url mqtt://localhost:1883 --sub cam/cam0/:/still --pub cam/cam0/:/vision
```

It analyzes images from `cam/cam0/:/still` and publishes results to `cam/cam0/:/vision`.

With additional options:

- `--show`: Popups a window to visualize current analyzed image and areas of detected faces;
- `--lbp`: Use LBP face detector which is fast, but less accurate.
