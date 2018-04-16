#include <fstream>
#include <stdexcept>
#include <memory>
#include <thread>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <glog/logging.h>
#include <mvnc.h>

#include "movidius/ncs.h"

namespace movidius {
    using namespace std;

    static void check_mvnc_status(mvncStatus s) {
        char str[128];
        sprintf(str, "mvnc: %d", s);

        switch (s) {
        case MVNC_INVALID_PARAMETERS: throw invalid_argument(str);
        case MVNC_OK: return;
        default: throw runtime_error(str);
        }
    }

    vector<string> NeuralComputeStick::devices() {
        vector<string> names;
        char name[128];
        int index = 0;
        while (mvncGetDeviceName(index, name, sizeof(name)) != MVNC_DEVICE_NOT_FOUND) {
            names.push_back(string(name));
            index ++;
        }
        return names;
    }

    NeuralComputeStick::NeuralComputeStick(const string& name)
    : m_handle(nullptr) {
        mvncStatus r = mvncOpenDevice(name.c_str(), &m_handle);
        check_mvnc_status(r);
    }

    NeuralComputeStick::~NeuralComputeStick() {
        if (m_handle != nullptr)
            mvncCloseDevice(m_handle);
    }

    NeuralComputeStick::Graph* NeuralComputeStick::allocGraph(const void *graph, size_t len) {
        void* handle = nullptr;
        mvncStatus r = mvncAllocateGraph(m_handle, &handle, graph, len);
        check_mvnc_status(r);
        return new Graph(handle);
    }

    NeuralComputeStick::Graph* NeuralComputeStick::loadGraphFile(const string& fn) {
        ifstream f;
        f.exceptions(ios::failbit|ios::badbit);
        f.open(fn, ios::in|ios::binary);
        f.seekg(0, f.end);
        auto len = f.tellg();
        unique_ptr<char> buf(new char[len]);
        if (buf == nullptr) throw runtime_error("not enough memory for graph file " + fn);
        f.seekg(0, f.beg);
        f.read(buf.get(), len);
        f.close();
        return allocGraph(buf.get(), len);
    }

    NeuralComputeStick::Graph::Graph(void* handle)
    : m_handle(handle) {

    }

    NeuralComputeStick::Graph::~Graph() {
        check_mvnc_status(mvncDeallocateGraph(m_handle));
    }

    void NeuralComputeStick::Graph::exec(const void* data, size_t len,
        function<void(const void*, size_t)> done) {
        check_mvnc_status(mvncLoadTensor(m_handle, data, len, nullptr));
        void *output = nullptr, *opaque = nullptr;
        unsigned int outlen = 0;
        check_mvnc_status(mvncGetResult(m_handle, &output, &outlen, &opaque));
        done(output, outlen);
    }

    float half2float(uint16_t fp16) {
        uint32_t e = fp16 & 0x7c00u;
        uint32_t sgn = ((uint32_t)fp16 & 0x8000u) << 16;
        uint32_t bits = 0;

        switch (e) {
        case 0:
            bits = fp16 & 0x03ffu;
            if (bits == 0) bits = sgn;
            else {
                bits <<= 1;
                while ((bits & 0x0400) == 0) {
                    bits <<= 1;
                    e ++;
                }
                e = ((uint32_t)(127 - 15 - e) << 23);
                bits = (bits & 0x03ff) << 13;
                bits += e + sgn;
            }
            break;
        case 0x7c00u:
            bits = sgn + 0x7f800000u + (((uint32_t)fp16 & 0x03ffu) << 13);
            break;
        default:
            bits = sgn + ((((uint32_t)fp16 & 0x7fffu) + 0x1c000u) << 13);
            break;
        }
        return *(float*)&bits;
    }

    vp::Graph::OpFunc NeuralComputeStick::Graph::op() {
        return [this] (vp::Graph::Ctx ctx) {
            const cv::Mat& m = ctx.in(0)->as<cv::Mat>();
            exec(m.data, m.rows * m.cols * 6, [ctx] (const void* out, size_t len) {
                len >>= 1;
                vector<float> fp(len);
                auto fp16 = (const uint16_t*)out;
                for (size_t i = 0; i < len; i ++) {
                    fp[i] = half2float(fp16[i]);
                }
                ctx.out(0)->set<vector<float>>(move(fp));
            });
        };
    }

    vp::Graph::OpFactory NeuralComputeStick::Graph::opFactory() {
        return [this] () -> vp::Graph::OpFunc {
            return op();
        };
    }

    uint16_t float2half(float fp32) {
        uint32_t u32 = *(uint32_t*)&fp32;

        // sign
        uint16_t sgn = (uint16_t)((u32 & 0x80000000) >> 16);
        // exponent
        uint32_t e = u32 & 0x7f800000;

        if (e >= 0x47800000) {
            return sgn | 0x7c00;
        } else if (e < 0x33000000) {
            return sgn;
        } else if (e <= 0x38000000) {
            e >>= 23;
            uint32_t bits = (0x00800000 + (u32 & 0x007fffff)) >> (113 - e);
            return sgn | (uint16_t)((bits + 0x00001000) >> 13);
        }

        return sgn +
            (uint16_t)((e - 0x38000000) >> 13) +
            (uint16_t)(((u32 & 0x007fffff) + 0x00001000) >> 13);
    }

    static void crop16fp_rows(const cv::Mat& src, cv::Mat& dst, int start, int rows) {
        int srcr0 = (src.rows - dst.rows) / 2;
        int srcc0 = (src.cols - dst.cols) / 2;
        for (int r = 0; r < rows; r ++) {
            uint16_t *p = (uint16_t*)dst.ptr(start+r);
            int sr = srcr0 + start + r;
            if (sr < 0 || sr >= src.rows) {
                memset(p, 0, dst.cols*2*3);
            } else {
                int sc = srcc0;
                for (int c = 0; c < dst.cols; c ++) {
                    if (sc < 0 || sc >= src.cols) {
                        memset(p, 0, 6);
                    } else {
                        uint8_t *sp = (uint8_t*)src.ptr(sr, sc);
                        for (int i = 0; i < 3; i ++)
                            p[i] = float2half(((float)(uint32_t)(sp[i]) - 127.5) * 0.007843);
                    }
                    sc ++;
                    p += 3;
                }
            }
        }
    }

    vp::Graph::OpFunc CropFp16Op::operator() () const {
        int w = cx, h = cy;
        return [w, h] (vp::Graph::Ctx ctx) {
            const cv::Mat& m = ctx.in(0)->as<cv::Mat>();
            ctx.out(0)->set<cv::Mat>(cropFp16(m, w, h));
        };
    }

    cv::Mat CropFp16Op::cropFp16(const cv::Mat &m, int cx, int cy) {
        cv::Mat m16(cy, cx, CV_16UC3);
        vector<unique_ptr<thread>> threads;
        int conc = 4;
        int rows = cy / conc;
        if (cy % conc) rows ++;
        for (int i = 0; i < conc; i ++) {
            int s = rows * i, n = rows;
            if (s + n > cy) n = cy - s;
            auto t = new thread([&m, &m16, s, n] () {
                crop16fp_rows(m, m16, s, n);
            });
            threads.push_back(move(unique_ptr<thread>(t)));
        }
        for (auto& pt : threads) pt->join();
        return m16;
    }

    void SSDMobileNet::Op::operator() (vp::Graph::Ctx ctx) {
        cv::Mat m = ctx.in(0)->as<cv::Mat>();
        int s = max(m.rows, m.cols);
        if (s != ImageSz) {
            cv::Mat d;
            double f = (double)ImageSz/s;
            cv::resize(m, d, cv::Size(), f, f);
            m = d;
        }
        cv::Mat m16 = CropFp16Op::cropFp16(m, ImageSz, ImageSz);
        graph->exec(m16.data, m16.total() * 6, [ctx] (const void* out, size_t len) {
            const cv::Mat& m = ctx.in(0)->as<cv::Mat>();
            ctx.out(0)->set<vector<vp::DetectBox>>(
                move(SSDMobileNet::parseResult(out, len, m.cols, m.rows)));
        });
    }

    vector<vp::DetectBox> SSDMobileNet::parseResult(
        const void* out, size_t len, int origCX, int origCY) {
        int s = max(origCX, origCY);
        int offx = (origCX - s)/2;
        int offy = (origCY - s)/2;
        vector<vp::DetectBox> boxes;
        auto fp16 = (const uint16_t*)out;
        len >>= 1;
        if (len > 7) {
            int count = (int)half2float(fp16[0]);
            if (count < 0) count = 0;
            if (count > 1000) count = 1000;
            for (int i = 0; i < count; i ++) {
                int off = (i+1)*7;
                if (off + 7 > len) break;
                vp::DetectBox box;
                box.category = (int)half2float(fp16[off+1]);
                box.confidence = half2float(fp16[off+2]);
                box.x0 = offx+(int)(half2float(fp16[off+3])*s);
                box.y0 = offy+(int)(half2float(fp16[off+4])*s);
                box.x1 = offx+(int)(half2float(fp16[off+5])*s);
                box.y1 = offy+(int)(half2float(fp16[off+6])*s);
                boxes.push_back(box);
            }
        }
        return boxes;
    }
}
