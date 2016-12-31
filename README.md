# Analytics Modules

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
