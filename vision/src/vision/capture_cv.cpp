#include "vision/app.h"

namespace vision {
    using namespace std;
    using namespace cv;
    using namespace cmn;

    class ImageWrapper : public RawImage {
    public:
        ImageWrapper(Mat* image, bool owned)
        : m_image(image), m_owned(owned) {
        }

        virtual ~ImageWrapper() {
            if (m_owned) {
                delete m_image;
            }
        }

        bool decode(Mat& out) const {
            if (m_image == nullptr) {
                return false;
            }
            out = *m_image;
            return !out.empty();
        }

        RawImage* clone() const {
            if (m_image == nullptr) {
                return new ImageWrapper(nullptr, false);
            }
            return new ImageWrapper(new Mat(*m_image), true);
        }

    private:
        Mat* m_image;
        bool m_owned;
    };

    OpenCVCapture::OpenCVCapture()
    : m_cap(nullptr) {

    }

    void OpenCVCapture::initialize(Application *app) {
        CaptureSource::initialize(app);
        app->options().add_options()
            ("cap", "capture image from source", cxxopts::value<string>())
        ;
    }

    void OpenCVCapture::start() {
        auto src = app()->opt("cap").as<string>();
        if (src.empty()) {
            return;
        }

        if (src.find_first_of("/dev/video") == 0) {
            m_cap = new VideoCapture(stoi(src.substr(10)));
        } else {
            m_cap = new VideoCapture(src);
        }
        if (!m_cap->isOpened()) {
            delete m_cap;
            m_cap = nullptr;
            return;
        }
        CaptureSource::start();
    }

    void OpenCVCapture::cleanup() {
        if (m_cap != nullptr) {
            delete m_cap;
            m_cap = nullptr;
        }
    }

    RawImage* OpenCVCapture::captureFrame() {
        if (m_cap->read(m_image)) {
            return new ImageWrapper(&m_image, false);
        }
        return nullptr;
    }
}
