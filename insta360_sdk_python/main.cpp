#pragma once
#include <iostream>
#include <thread>
#include "main.h"

#include <camera/camera.h>  
#include <camera/photography_settings.h>
#include <camera/device_discovery.h>
#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/function.hpp>
#include <regex>
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <tuple>
#include <opencv2/opencv.hpp>
#include <opencv2/video/video.hpp>
#include <numeric>
extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <stdio.h>
}

std::list<std::vector<uint8_t>> buffer;
bool flag = false;
int a(void* ptr, uint8_t* buf, int buf_size) {
    if (buffer.size()<32||flag==false) {
        //printf("aaa");
        return AVERROR_EOF;
    }
    else {
        int cur = 0;
        for (auto b : buffer) {
            std::vector<uint8_t> bufBuffer = b;
            memcpy(buf+cur, bufBuffer.data(), bufBuffer.size() * sizeof(uint8_t));
            cur += bufBuffer.size();
            //printf("b%d\n", bufBuffer.size());
        }
        buffer.erase(buffer.begin(),std::next(buffer.begin(),1));
        flag = false;
        return cur;
    }
};
static void avlog_cb(void*, int level, const char* szFmt, va_list varg) {

}
std::vector<uint8_t> vec;
class TestStreamDelegate : public ins_camera::StreamDelegate {
public:
    boost::python::numpy::ndarray array_ = boost::python::numpy::zeros(boost::python::make_tuple((1,1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
    TestStreamDelegate() {
        av_log_set_callback(avlog_cb);
        file1_ = fopen("./01.h264", "wb");
        file2_ = fopen("./02.h264", "wb");
    }
    ~TestStreamDelegate() {
        fclose(file1_);
        fclose(file2_);
    }

    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {
        //std::cout << "on audio data:" << std::endl;
    }
    int iii = 0;
    void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index = 0) override {
        //std::cout << "on video frame:" << size << ";" << timestamp << std::endl;
        iii++;
        std::vector<uint8_t> aaa(data, data + size);
        //if(buffer.size()<20)
        buffer.push_back(aaa);
        if (buffer.size() > 32) {
            buffer.pop_front();
        }
        //if (buff_ == NULL) {
        //vec.clear();
            //std::list<uint8_t> aaaa(data, data+size);
            //aaaa.insert(aaaa.end(), aaaa.begin(), aaaa.end());
        //}
        //printf("p\n");
        //return;
        //if (iii % 60 != 0) {
            //return;
        //}
        //printf("%d,%d\n", vec.size(), aaa.size());
        //return;
        //return;
        /*if (stream_index == 0) {
            fwrite(data, sizeof(uint8_t), size, file1_);
        }
        if (stream_index == 1) {
            fwrite(data, sizeof(uint8_t), size, file2_);
        }*/
    }
    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {

        //for (auto& gyro : data) {
        //	if (gyro.timestamp - last_timestamp > 2) {
        //		fprintf(file1_, "timestamp:%lld package_size = %d  offtimestamp = %lld gyro:[%f %f %f] accel:[%f %f %f]\n", gyro.timestamp, data.size(), gyro.timestamp - last_timestamp, gyro.gx, gyro.gy, gyro.gz, gyro.ax, gyro.ay, gyro.az);
        //	}
        //	last_timestamp = gyro.timestamp;
        //}
    }
    void OnExposureData(const ins_camera::ExposureData& data) override {
        //fprintf(file2_, "timestamp:%lld shutter_speed_s:%f\n", data.timestamp, data.exposure_time);
    }


private:
    FILE* file1_;
    FILE* file2_;
    int64_t last_timestamp = 0;
};


// wrapper class for ins_camera::Camera
class _Camera {
public:
    /**
     * \brief see also DeviceDiscovery to get DeviceConnectionInfo.
     */
    std::shared_ptr<TestStreamDelegate> streamDelegate = std::make_shared<TestStreamDelegate>();
    _Camera(const ins_camera::DeviceConnectionInfo& info)
        :impl(std::make_shared<ins_camera::Camera>(info)) {
        auto p = std::dynamic_pointer_cast<ins_camera::StreamDelegate>(streamDelegate);
        SetStreamDelegate(p);
        //auto array_ = boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
    }
    _Camera(const int cameraIdx)
        :impl(std::make_shared<ins_camera::Camera>(
            ins_camera::DeviceDiscovery().GetAvailableDevices()[cameraIdx].info)) {
        //std::shared_ptr<ins_camera::StreamDelegate> streamDelegate = std::make_shared<TestStreamDelegate>();
        auto p = std::dynamic_pointer_cast<ins_camera::StreamDelegate>(streamDelegate);
        SetStreamDelegate(p);
    }
    boost::python::numpy::ndarray Read() {
        // Allocate a AVContext
        AVFormatContext* formatContext = avformat_alloc_context();
        int bufsize = std::accumulate(buffer.begin(), buffer.end(), 0, [](int acc, auto i) {return acc + i.size(); });
        // Alloc a buffer for the stream
        unsigned char* fileStreamBuffer = (unsigned char*)av_malloc(100 * bufsize * sizeof(uint8_t));
        flag = true;
        AVIOContext* ioContext = avio_alloc_context(
            fileStreamBuffer,
            bufsize,
            0,
            NULL,
            &a,
            NULL,
            NULL
        );
        if (ioContext == NULL) {
            printf("avcio_alloc_context failed\n");
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //vec.clear();

        // Set up the Format Context
        formatContext->pb = ioContext;
        formatContext->flags |= AVFMT_FLAG_CUSTOM_IO; // we set up our own IO

        if (avformat_open_input(&formatContext, "", nullptr, nullptr) < 0) {
            // Error opening file
            //ここでエラー
            printf("avformat_open_input failed\n");
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }

        //av_free(ioContext);
        //printf("abc");
        // get stream info
        if (avformat_find_stream_info(formatContext, nullptr) < 0) {
            printf("avformat_find_stream_info failed\n");
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }

        //return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());

        //printf("def");
        AVStream* videoStream = nullptr;
        for (int i = 0; i < (int)formatContext->nb_streams; ++i) {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStream = formatContext->streams[i];
                break;
            }
        }
        if (videoStream == nullptr) {
            printf("No video stream ...\n");
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //printf("efg");
        // find decoder
        const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (codec == nullptr) {
            printf("No supported decoder ...\n");
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //printf("fgh");
        // alloc codec context
        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        if (codecContext == nullptr) {
            printf("avcodec_alloc_context3 failed\n");
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //printf("ghi");
        // open codec
        if (avcodec_parameters_to_context(codecContext, videoStream->codecpar) < 0) {
            printf("avcodec_parameters_to_context failed\n");
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //printf("hij");
        if (avcodec_open2(codecContext, codec, nullptr) != 0) {
            printf("avcodec_open2 failed\n");
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //printf("ijk");
        // decode frames
        AVFrame* frame = av_frame_alloc();
        AVFrame* frameRGBA = nullptr;
        AVPacket packet = AVPacket();
        struct SwsContext* swsContext = sws_getContext
        (
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,
            codecContext->width,
            codecContext->height,
            AV_PIX_FMT_BGRA,
            0,
            NULL,
            NULL,
            NULL
        );
        if (swsContext == nullptr) {
            printf("sws context failed\n");
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());

        }
        //return//
        while (av_read_frame(formatContext, &packet) == 0) {
            //return array_;
            if (packet.stream_index == videoStream->index) {

                if (avcodec_send_packet(codecContext, &packet) != 0) {
                    //printf("avcodec_send_packet failed\n");
                    //printf("avcodec_send_packet failed\n");
                    continue;
                }
                //printf("jkl");
                //continue;
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    if (frameRGBA == nullptr) {
                        frameRGBA = av_frame_alloc();
                    }
                    //on_frame_decoded(frame);
                    //get the scaling context
                    //printf("klm");
                    // Convert the image from its native format to RGBA
                    frameRGBA->height = frame->height;
                    frameRGBA->width = frame->width;
                    frameRGBA->format = AV_PIX_FMT_RGBA;
                    if (av_image_alloc(frameRGBA->data, frameRGBA->linesize, frame->width, frame->height, AV_PIX_FMT_BGRA, 32) < 0) {
                        printf("image alloc error");
                        continue;
                    }
                    sws_scale
                    (
                        swsContext,
                        (uint8_t const* const*)frame->data,
                        frame->linesize,
                        0,
                        frame->height,
                        frameRGBA->data,
                        frameRGBA->linesize
                    );
                }
            }
            av_packet_unref(&packet);
        }

        //return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        if (frameRGBA == nullptr) {

            printf("get frame failed");

            sws_freeContext(swsContext);
            av_frame_free(&frame);
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        auto size___ = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frameRGBA->width, frameRGBA->height, 1);
        if (size___ < 0) {
            printf("av_image_get_buffer_size failed");

            sws_freeContext(swsContext);
            av_frame_free(&frame);
            av_frame_free(&frameRGBA);
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            av_free(ioContext);
            free(fileStreamBuffer);
            return boost::python::numpy::zeros(boost::python::make_tuple((1, 1)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        }
        //Release
        uint8_t* pt = (uint8_t*)malloc(size___);
        //int8_t ppt[1280 * 1280 * 4];
        av_image_copy_to_buffer((uint8_t*)pt, size___, frameRGBA->data, frameRGBA->linesize, AV_PIX_FMT_RGBA, frameRGBA->width, frameRGBA->height, 1);
        //memcpy(pt, frame->data[AV_NUM_DATA_POINTERS], frame->linesize[AV_NUM_DATA_POINTERS]);
        auto array_ = boost::python::numpy::zeros(boost::python::make_tuple((frameRGBA->width * frameRGBA->height * 4)), boost::python::numpy::dtype::get_builtin<int>());
        //memcpy(array_.get_data(), pt, frameRGBA->width * frameRGBA->height * 4);
        for (int i = 0; i < frameRGBA->width * frameRGBA->height * 4; ++i) {
            array_[i] = pt[i];
        }
        //array_ = boost::python::numpy::from_data((void*)pt, boost::python::numpy::dtype::get_builtin<uint8_t>(), boost::python::make_tuple((frameRGBA->width)), boost::python::make_tuple((sizeof(uint8_t))), boost::python::object());

        //array_ = boost::python::numpy::zeros(boost::python::make_tuple((1, 2)), boost::python::numpy::dtype::get_builtin<uint8_t>());
        printf("%d,%d,%d\n", frameRGBA->height, frameRGBA->width, frameRGBA->channels);
        printf("grok%d,%d,%d\n", frame->height, frame->width, frame->channels);
        printf("grok%d,%d,%d\n", codecContext->height, codecContext->width, codecContext->channels);
        free(pt);

        sws_freeContext(swsContext);
        av_frame_free(&frame);
        av_frame_free(&frameRGBA);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        av_free(ioContext);
        free(fileStreamBuffer);
        //array_.reshape(boost::python::tuple((frameRGBA->height, frameRGBA->width, 4))); 

        return array_;
    }
    void SetCallback(boost::function<void(boost::python::numpy::ndarray)> callback) {

    }
    /**
     * \brief Open camera and start session
     * \return true if succeed, otherwise return false;
     */
    bool Open() const {
        return impl->Open();
    }
    /**
     * \brief Close camera and release resources
     */
    void Close() const {
        return impl->Close();
    }
    /**
     * \brief Get the serial number from camera
     * \return The serial number string
     */
    std::string GetSerialNumber() const {
        return impl->GetSerialNumber();
    }

    /**
     * \brief get type of camera. Camera type are models like OneX, OneR
     * be sure to call this function after Open()
     * \return lens type of the camera
     */
    ins_camera::CameraType GetCameraType() const {
        return impl->GetCameraType();
    }

    /**
     * \brief get lens type of camera. For model earlier than OneR, the lens type will be PanoDefault
     * be sure to call this function after Open()
     * \return lens type of the camera
     */
    ins_camera::CameraLensType GetCameraLensType() const {
        return impl->GetCameraLensType();
    }

    /**
    * \brief get uuid of camera. For model earlier than OneR, the lens type will be PanoDefault
    * be sure to call this function after Open()
    * \return lens type of the camera
    */
    std::string GetCameraUUID() const {
        return impl->GetCameraUUID();
    }

    /**
    * \brief get capture current status of camera.
    * be sure to call this function after Open()
    * \return lens type of the camera
    */
    ins_camera::CaptureStatus GetCaptureCurrentStatus() const {
        return impl->GetCaptureCurrentStatus();
    }

    /**
    * \brief get offset of camera
    * be sure to call this function after Open()
    */
    std::string GetCameraOffset() const {
        return impl->GetCameraOffset();
    }

    /**
    * \brief get current camera encode type(h264 or h265)
    */
    ins_camera::VideoEncodeType GetVideoEncodeType() const {
        return impl->GetVideoEncodeType();
    }

    /**
     * \brief Control camera to take photo
     * \return The url of the photo if success, otherwise empty
     */
    ins_camera::MediaUrl TakePhoto() const {
        return impl->TakePhoto();
    }

    /**
     * \brief set exposure settings, the settings will only be applied to specified mode
     * \param mode the target mode you want to apply exposure settings, for normal video recording,
     *      use CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO, for normal still image capture,
     *      use CaperaFunctionMode::FUNCTION_MODE_NORMAL_IMAGE.
     * \param settings ExposureSettings containing exposure mode/iso/shutter/ev to be applied.
     * \return true on success, false otherwise.
     */
    bool SetExposureSettings(ins_camera::CameraFunctionMode mode, std::shared_ptr<ins_camera::ExposureSettings> settings) {
        return impl->SetExposureSettings(mode, settings);
    }


    std::shared_ptr<ins_camera::ExposureSettings> GetExposureSettings(ins_camera::CameraFunctionMode mode) const {
        return impl->GetExposureSettings(mode);
    }

    /**
     * \brief set capture settings, the settings will only be applied to specified mode
     * \param mode the target mode you want to apply exposure settings, for normal video recording,
     *      use CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO, for normal still image capture,
     *      use CaperaFunctionMode::FUNCTION_MODE_NORMAL_IMAGE. 
     * \param settings CaptureSettings containing capture settings like saturation,contrast,whitebalance,sharpness,brightness and etc.
     * \return true on success, false otherwise.
     */
    bool SetCaptureSettings(ins_camera::CameraFunctionMode mode, std::shared_ptr<ins_camera::CaptureSettings> settings) {
        return impl->SetCaptureSettings(mode, settings);
    }

    std::shared_ptr<ins_camera::CaptureSettings> GetCaptureSettings(ins_camera::CameraFunctionMode mode) const {
        return impl->GetCaptureSettings(mode);
    }

    /**
     * \brief set capture settings such as resolutions, bitrate,
     * \param params RecordParams containing settings you want to apply
     * \param mode the target mode you want to apply capture settings, the mode must be one of video modes.
     * \return true on success, false otherwise.
     */
    bool SetVideoCaptureParams(ins_camera::RecordParams params, ins_camera::CameraFunctionMode mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO) {
        return impl->SetVideoCaptureParams(params, mode);
    }

    /**
     * \brief Start Recording.
     * \return true on success, false otherwise
     */
    bool StartRecording() {
        return impl->StartRecording();
    }

    /**
     * \brief Stop Recording.
     * \return video uri of the video, it may contain low bitrate proxy video
     */
    ins_camera::MediaUrl StopRecording() {
        return impl->StopRecording();
    }

    /**
    * \brief start preview stream
    */
    bool StartLiveStreaming(const ins_camera::LiveStreamParam& param) {
        return impl->StartLiveStreaming(param);
    }

    /**
    * \brief stop preview stream
    */
    bool StopLiveStreaming() {
        return impl->StopLiveStreaming();
    }

    /**
        \brief set a stream delegate, you may implement StreamDelegate to handle stream data.
     */
    void SetStreamDelegate(std::shared_ptr<ins_camera::StreamDelegate>& delegate) {
        return impl->SetStreamDelegate(delegate);
    }

    /**
     * \brief Delete the specified file from camera
     */
    bool DeleteCameraFile(const std::string& filePath) const {
        return impl->DeleteCameraFile(filePath);
    }

    /**
     * \brief Download the specified file from camera
     */
    bool DownloadCameraFile(const std::string& remoteFilePath, const std::string& localFilePath) const {
        return impl->DownloadCameraFile(remoteFilePath, localFilePath);
    }

    /**
     * \brief Get the list of files stored in camera storage card
     */
    std::vector<std::string> GetCameraFilesList() const {
        return impl->GetCameraFilesList();
    }

    /**
    * \brief set timelapse param
    */
    bool SetTimeLapseOption(ins_camera::TimelapseParam params) {
        return impl->SetTimeLapseOption(params);
    }

    /**
    * \brief start timelapse
    */
    bool StartTimeLapse(ins_camera::CameraTimelapseMode mode) {
        return impl->StartTimeLapse(mode);
    }

    /**
    * \brief stop timelapse
    * \return video uri of the video, it may contain low bitrate proxy video
    */
    ins_camera::MediaUrl StopTimeLapse(ins_camera::CameraTimelapseMode mode) {
        return impl->StopTimeLapse(mode);
    }

    /**
    * \brief sync local time to camera
    */
    bool SyncLocalTimeToCamera(uint64_t time) {
        return impl->SyncLocalTimeToCamera(time);
    }

    /**
     * \brief get http base url with trailing slash. eg. "http://127.0.0.1:9099/",
     *  you could get url of camera file by concanate base url and file uri
     */
    std::string GetHttpBaseUrl() const {
        return impl->GetHttpBaseUrl();
    }

private:
    const std::shared_ptr<ins_camera::Camera> impl;
};

// https://stackoverflow.com/questions/36049533/sending-python-function-as-boost-function-argument
struct BoostFunc_from_Python_Callable
{
    BoostFunc_from_Python_Callable()
    {
        boost::python::converter::registry::push_back(&convertible, &construct, boost::python::type_id< boost::function< void(boost::python::numpy::ndarray) > >());
    }

    static void* convertible(PyObject* obj_ptr)
    {
        if (!PyCallable_Check(obj_ptr))
            return 0;
        return obj_ptr;
    }

    static void construct(PyObject* obj_ptr, boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        boost::python::object callable(boost::python::handle<>(boost::python::borrowed(obj_ptr)));
        void* storage = ((boost::python::converter::rvalue_from_python_storage< boost::function< void(boost::python::numpy::ndarray) > >*) data)->storage.bytes;
        new (storage)boost::function< void(boost::python::numpy::ndarray) >(callable);
        data->convertible = storage;
    }
};

BOOST_PYTHON_MODULE(insta360_sdk_python)
{
    using namespace boost::python;
    Py_Initialize();
    numpy::initialize();

    // Register function converter
    BoostFunc_from_Python_Callable();


    //ins_types.h
    boost::python::enum_<ins_camera::CameraType>("CameraType")
        .value("unknown", ins_camera::CameraType::Unknown)
        .value("Insta360OneX", ins_camera::CameraType::Insta360OneX)
        .value("Insta360OneR", ins_camera::CameraType::Insta360OneR)
        .value("Insta360OneX2", ins_camera::CameraType::Insta360OneX2)
        ;// .export_values();
    boost::python::enum_<ins_camera::CameraLensType>("CameraLensType")
        .value("PanoDefault", ins_camera::CameraLensType::PanoDefault)
        .value("Wide577", ins_camera::CameraLensType::Wide577)
        .value("Pano577", ins_camera::CameraLensType::Pano577)
        .value("Wide283", ins_camera::CameraLensType::WIDE283)
        ;// .export_values();
    boost::python::enum_<ins_camera::ConnectionType>("ConnectionType")
        .value("USB", ins_camera::ConnectionType::USB)
        .value("Wifi", ins_camera::ConnectionType::Wifi)
        .value("Bluetooth", ins_camera::ConnectionType::Bluetooth)
        ;// .export_values();
    boost::python::enum_<ins_camera::VideoEncodeType>("VideoEncodeType")
        .value("H264", ins_camera::VideoEncodeType::H264)
        .value("H265", ins_camera::VideoEncodeType::H265)
        ;// .export_values();
    class_<ins_camera::DeviceConnectionInfo>("DeviceConnectionInfo")
        .add_property("connection_type", &ins_camera::DeviceConnectionInfo::connection_type)
        .add_property("native_connection_info", &ins_camera::DeviceConnectionInfo::native_connection_info);
    class_<ins_camera::DeviceDescriptor>("DeviceDescriptor")
        .add_property("camera_type", &ins_camera::DeviceDescriptor::camera_type)
        .add_property("lens_type", &ins_camera::DeviceDescriptor::lens_type)
        .add_property("serial_number", &ins_camera::DeviceDescriptor::serial_number)
        .add_property("info", &ins_camera::DeviceDescriptor::info);
    class_<ins_camera::MediaUrl>("MediaUrl", init<std::vector<std::string>, std::vector<std::string>>())
        .def("empty", &ins_camera::MediaUrl::Empty)
        .def("is_single_origin", &ins_camera::MediaUrl::IsSingleOrigin)
        .def("is_single_lrv", &ins_camera::MediaUrl::IsSingleLRV)
        .def("get_single_origin", &ins_camera::MediaUrl::GetSingleOrigin)
        .def("get_single_lrv", &ins_camera::MediaUrl::GetSingleLRV)
        .def("origin_urls", &ins_camera::MediaUrl::OriginUrls, return_value_policy<return_by_value>())
        .def("lrv_urls", &ins_camera::MediaUrl::LRVUrls, return_value_policy<return_by_value>());
        


    //device_discovery.h
    class_<std::vector<std::string>>("vector_string")
        .def(vector_indexing_suite<std::vector<std::string>>());
    class_<std::vector<ins_camera::DeviceDescriptor>>("vector_DeviceDiscriptor")
        .def(vector_indexing_suite<std::vector<ins_camera::DeviceDescriptor>>());
    class_<ins_camera::DeviceDiscovery>("DeviceDiscovery")
        .def("get_available_devices", &ins_camera::DeviceDiscovery::GetAvailableDevices);


    //camera.h
    class_<_Camera>("Camera", init<ins_camera::DeviceConnectionInfo>())
        .def("read", &_Camera::Read)
        .def("open", &_Camera::Open)
        .def("close", &_Camera::Close)
        .def("get_serial_number", &_Camera::GetSerialNumber)
        .def("get_camera_type", &_Camera::GetCameraType)
        .def("get_camera_lens_type", &_Camera::GetCameraLensType)
        .def("get_camera_uuid", &_Camera::GetCameraUUID)
        .def("get_capture_current_status", &_Camera::GetCaptureCurrentStatus)
        .def("get_camera_offset", &_Camera::GetCameraOffset)
        .def("get_video_encode_type", &_Camera::GetVideoEncodeType)
        .def("take_photo", &_Camera::TakePhoto)
        .def("set_exposure_settings", &_Camera::SetExposureSettings)
        .def("get_exposure_settings", &_Camera::GetExposureSettings)
        .def("set_caputure_settings", &_Camera::SetCaptureSettings)
        .def("get_capture_settings", &_Camera::GetCaptureSettings)
        .def("set_video_capture_params", &_Camera::SetVideoCaptureParams)
        .def("start_recording", &_Camera::StartRecording)
        .def("stop_recording", &_Camera::StopRecording)
        .def("start_live_streaming", &_Camera::StartLiveStreaming)
        .def("stop_live_streaming", &_Camera::StopLiveStreaming)
        .def("set_stream_delegate", &_Camera::SetStreamDelegate)
        .def("delete_camera_file", &_Camera::DeleteCameraFile)
        .def("download_camera_file", &_Camera::DownloadCameraFile)
        .def("get_camera_file_list", &_Camera::GetCameraFilesList)
        .def("set_timelapse_option", &_Camera::SetTimeLapseOption)
        .def("start_timelapse", &_Camera::StartTimeLapse)
        .def("stop_timelapse", &_Camera::StopTimeLapse)
        .def("sync_local_time_to_camera", &_Camera::SyncLocalTimeToCamera)
        .def("get_http_base_url", &_Camera::GetHttpBaseUrl)
        .def("set_callback", &_Camera::SetCallback);


    //photography_settings.h
    enum_<ins_camera::CameraFunctionMode>("CameraFunctionMode")
        .value("FUNCTION_MODE_NORMAL", ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL)
        .value("FUNCTION_MODE_LIVE_STREAM", ins_camera::CameraFunctionMode::FUNCTION_MODE_LIVE_STREAM)
        .value("FUNCTION_MODE_NORMAL_IMAGE", ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE)
        .value("FUNCTION_MODE_NORMAL_VIDEO", ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO)
        .value("FUNCTION_MODE_STATIC_TIMELAPSE", ins_camera::CameraFunctionMode::FUNCTION_MODE_STATIC_TIMELAPSE)
        .value("FUNCTION_MODE_MOBILE_TIMELAPSE", ins_camera::CameraFunctionMode::FUNCTION_MODE_MOBILE_TIMELAPSE)
        ;// .export_values();
    enum_<ins_camera::CameraTimelapseMode>("CameraTimelapseMode")
        .value("TIMELAPSE_MIXED", ins_camera::CameraTimelapseMode::TIMELAPSE_MIXED)
        .value("MOBILE_TIMELAPSE_VIDEO", ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO)
        .value("TIMELAPSE_INTERVAL_SHOOTING", ins_camera::CameraTimelapseMode::TIMELAPSE_INTERVAL_SHOOTING)
        .value("STATIC_TIMELAPSE_VIDEO", ins_camera::CameraTimelapseMode::STATIC_TIMELAPSE_VIDEO)
        .value("TIMELAPSE_INTERVAL_VIDEO", ins_camera::CameraTimelapseMode::TIMELAPSE_INTERVAL_VIDEO)
        .value("TIMELAPSE_STARTLAPSE_SHOOTING", ins_camera::CameraTimelapseMode::TIMELAPSE_STARLAPSE_SHOOTING)
        ;// .export_values();
    enum_<ins_camera::CaptureStatus>("CaptureStatus")
        .value("NOT_CAPTURE", ins_camera::CaptureStatus::NOT_CAPTURE)
        .value("NORMAL_CAPTURE", ins_camera::CaptureStatus::NORMAL_CAPTURE)
        .value("TIMELAPSE_CAPTURE", ins_camera::CaptureStatus::TIMELAPSE_CAPTURE)
        .value("INTERVAL_SHOOTING_CAPTURE", ins_camera::CaptureStatus::INTERVAL_SHOOTING_CAPTURE)
        .value("SINGLE_SHOOTING", ins_camera::CaptureStatus::SINGLE_SHOOTING)

        .value("HDR_SHOOTING", ins_camera::CaptureStatus::HDR_SHOOTING)
        .value("SELF_TIMER_SHOOTING", ins_camera::CaptureStatus::SELF_TIMER_SHOOTING)
        .value("BULLET_TIME_CAPTURE", ins_camera::CaptureStatus::BULLET_TIME_CAPTURE)
        .value("SETTING_NEW_VALUE", ins_camera::CaptureStatus::SETTINGS_NEW_VALUE)
        .value("HDR_CAPTURE", ins_camera::CaptureStatus::HDR_CAPTURE)

        .value("BURST_SHOOTING", ins_camera::CaptureStatus::BURST_SHOOTING)
        .value("STATIC_TIMELAPSE_SHOOTING", ins_camera::CaptureStatus::STATIC_TIMELAPSE_SHOOTING)
        .value("INTERVAL_VIDEO_CAPTURE", ins_camera::CaptureStatus::INTERVAL_VIDEO_CAPTURE)
        .value("TIMESHIFT_CAPTURE", ins_camera::CaptureStatus::TIMESHIFT_CAPTURE)
        .value("AEB_NIGHT_SHOOTING", ins_camera::CaptureStatus::AEB_NIGHT_SHOOTING)

        .value("SINGLE_POWER_PANO_SHOOTING", ins_camera::CaptureStatus::SINGLE_POWER_PANO_SHOOTING)
        .value("HDR_POWER_PANO_SHOOTING", ins_camera::CaptureStatus::HDR_POWER_PANO_SHOOTING)
        .value("SUPER_NORMAL_CAPTURE", ins_camera::CaptureStatus::SUPER_NORMAL_CAPTURE)
        .value("LOOP_RECORDING_CAPTURE", ins_camera::CaptureStatus::LOOP_RECORDING_CAPTURE)
        .value("STARLAPSE_SHOOTING", ins_camera::CaptureStatus::STARLAPSE_SHOOTING)
        ;// .export_values();
        enum_<ins_camera::VideoResolution>("VideoResolution")
            //1
            .value("RES_3840_1920P30", ins_camera::VideoResolution::RES_3840_1920P30)
            .value("RES_2560_1280P30", ins_camera::VideoResolution::RES_2560_1280P30)
            .value("RES_1920_960P30", ins_camera::VideoResolution::RES_1920_960P30)
            .value("RES_2560_1280P60", ins_camera::VideoResolution::RES_2560_1280P60)
            .value("RES_2048_512P120", ins_camera::VideoResolution::RES_2048_512P120)

            .value("RES_3328_832P60", ins_camera::VideoResolution::RES_3328_832P60)
            .value("RES_3072_1536P30", ins_camera::VideoResolution::RES_3072_1536P30)
            .value("RES_2240_1120P30", ins_camera::VideoResolution::RES_2240_1120P30)
            .value("RES_2240_1120P24", ins_camera::VideoResolution::RES_2240_1120P24)
            .value("RES_1440_720P30", ins_camera::VideoResolution::RES_1440_720P30)

            //2
            .value("RES_2880_2880P30", ins_camera::VideoResolution::RES_2880_2880P30)
            .value("RES_3840_1920P60", ins_camera::VideoResolution::RES_3840_1920P60)
            .value("RES_3840_1920P50", ins_camera::VideoResolution::RES_3840_1920P60)
            .value("RES_3008_1504P100", ins_camera::VideoResolution::RES_3008_1504P100)
            .value("RES_960_480P30", ins_camera::VideoResolution::RES_960_480P30)

            .value("RES_3040_1520P30", ins_camera::VideoResolution::RES_3040_1520P30)
            .value("RES_2176_1088P30", ins_camera::VideoResolution::RES_2176_1088P30)
            .value("RES_720_360P30", ins_camera::VideoResolution::RES_720_360P30)
            .value("RES_480_240P30", ins_camera::VideoResolution::RES_480_240P30)
            .value("RES_2880_2880P25", ins_camera::VideoResolution::RES_2880_2880P25)

            //3
            .value("RES_2880_2880P24", ins_camera::VideoResolution::RES_2880_2880P24)
            .value("RES_3840_1920P20", ins_camera::VideoResolution::RES_3840_1920P20)
            .value("RES_1920_960P20", ins_camera::VideoResolution::RES_1920_960P20)
            .value("RES_3840_2160p60", ins_camera::VideoResolution::RES_3840_2160p60)
            .value("RES_3840_2160p30", ins_camera::VideoResolution::RES_3840_2160p30)

            .value("RES_2720_1530p100", ins_camera::VideoResolution::RES_2720_1530p100)
            .value("RES_1920_1080p200", ins_camera::VideoResolution::RES_1920_1080p200)
            .value("RES_1920_1080p240", ins_camera::VideoResolution::RES_1920_1080p240)
            .value("RES_1920_1080p120", ins_camera::VideoResolution::RES_1920_1080p120)
            .value("RES_1920_1080p30", ins_camera::VideoResolution::RES_1920_1080p30)

            //4
            .value("RES_5472_3078p30", ins_camera::VideoResolution::RES_5472_3078p30)
            .value("RES_4000_3000p30", ins_camera::VideoResolution::RES_4000_3000p30)
            .value("RES_854_640P30", ins_camera::VideoResolution::RES_854_640P30)
            .value("RES_720_406P30", ins_camera::VideoResolution::RES_720_406P30)
            .value("RES_424_240P15", ins_camera::VideoResolution::RES_424_240P15)

            .value("RES_1024_512P30", ins_camera::VideoResolution::RES_1024_512P30)
            .value("RES_640_320P30", ins_camera::VideoResolution::RES_640_320P30)
            .value("RES_5312_2988P30", ins_camera::VideoResolution::RES_5312_2988P30)
            .value("RES_2720_1530P60", ins_camera::VideoResolution::RES_2720_1530P60)
            .value("RES_2720_1530P30", ins_camera::VideoResolution::RES_2720_1530P30)

            //5
            .value("RES_1920_1080P60", ins_camera::VideoResolution::RES_1920_1080P60)
            .value("RES_2720_2040P30", ins_camera::VideoResolution::RES_2720_2040P30)
            .value("RES_1920_1440P30", ins_camera::VideoResolution::RES_1920_1440P30)
            .value("RES_1280_720P30", ins_camera::VideoResolution::RES_1280_720P30)
            .value("RES_1280_960P30", ins_camera::VideoResolution::RES_1280_960P30)

            .value("RES_1152_768P30", ins_camera::VideoResolution::RES_1152_768P30)
            .value("RES_5312_2988P25", ins_camera::VideoResolution::RES_5312_2988P25)
            .value("RES_5312_2988P24", ins_camera::VideoResolution::RES_5312_2988P24)
            .value("RES_3840_2160P25", ins_camera::VideoResolution::RES_3840_2160P25)
            .value("RES_3840_2160P24", ins_camera::VideoResolution::RES_3840_2160P24)

            //6
            .value("RES_2720_1530P25", ins_camera::VideoResolution::RES_2720_1530P25)
            .value("RES_2720_1530P24", ins_camera::VideoResolution::RES_2720_1530P24)
            .value("RES_1920_1080P25", ins_camera::VideoResolution::RES_1920_1080P25)
            .value("RES_1920_1080P24", ins_camera::VideoResolution::RES_1920_1080P24)
            .value("RES_4000_3000P25", ins_camera::VideoResolution::RES_4000_3000P25)

            .value("RES_4000_3000P24", ins_camera::VideoResolution::RES_4000_3000P24)
            .value("RES_2720_2040P25", ins_camera::VideoResolution::RES_2720_2040P25)
            .value("RES_2720_2040P24", ins_camera::VideoResolution::RES_2720_2040P24)
            .value("RES_1920_1440P25", ins_camera::VideoResolution::RES_1920_1440P25)
            .value("RES_1920_1440P24", ins_camera::VideoResolution::RES_1920_1440P24)
            ;// .export_values();
    enum_<ins_camera::PhotographyOptions_ExposureMode>("PhotographyOptions_ExposureMode")
        .value("PhotographyOptions_ExposureOptions_Program_AUTO", ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_AUTO)
        .value("PhotographyOptions_ExposureOptions_Program_ISO_PRIORITY", ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_ISO_PRIORITY)
        .value("PhotographyOptions_ExposureOptions_Program_SHUTIER_PRIORITY", ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_SHUTTER_PRIORITY)
        .value("PhotographyOptions_ExposureOptions_Program_MANUAL", ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_MANUAL)
        ;// .export_values();
    enum_<ins_camera::PhotographyOptions_WhiteBalance>("PhotographyOptions_WhiteBalance")
        .value("PhotographyOption_WhiteBalance_WB_UNKNOWN", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_UNKNOWN)
        .value("PhotographyOption_WhiteBalance_WB_AUTO", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_AUTO)
        .value("PhotographyOption_WhiteBalance_WB_2700K", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_2700K)
        .value("PhotographyOption_WhiteBalance_WB_4000K", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_4000K)
        .value("PhotographyOption_WhiteBalance_WB_5000K", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_5000K)
        .value("PhotographyOption_WhiteBalance_WB_6500K", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_6500K)
        .value("PhotographyOption_WhiteBalance_WB_7500K", ins_camera::PhotographyOptions_WhiteBalance::PhotographyOptions_WhiteBalance_WB_7500K)
        ;// .export_values();
    class_<ins_camera::ExposureSettings>("ExposureSettings")
        .def("set_iso", &ins_camera::ExposureSettings::SetIso)
        .def("set_shutter_speed", &ins_camera::ExposureSettings::SetShutterSpeed)
        .def("set_exposure_mode", &ins_camera::ExposureSettings::ExposureMode)
        .def("set_ev_bias", &ins_camera::ExposureSettings::SetEVBias)
        .def("get_iso", &ins_camera::ExposureSettings::Iso)
        .def("get_shutter_speed", &ins_camera::ExposureSettings::ShutterSpeed)
        .def("get_exposure_mode", &ins_camera::ExposureSettings::ExposureMode)
        .def("get_ev_bias", &ins_camera::ExposureSettings::EVBias);


    {
        scope outer =
            class_<ins_camera::CaptureSettings>("CaptureSettings")
            .def("get_setting_types", &ins_camera::CaptureSettings::GetSettingTypes)
            .def("update_setting_types", &ins_camera::CaptureSettings::UpdateSettingTypes)
            .def("reset_setting_types", &ins_camera::CaptureSettings::ResetSettingTypes)
            .def("set_value", &ins_camera::CaptureSettings::SetValue)
            .def("set_white_balance", &ins_camera::CaptureSettings::SetWhiteBalance)
            .def("get_int_value", &ins_camera::CaptureSettings::GetIntValue)
            .def("get_white_balance", &ins_camera::CaptureSettings::WhiteBalance);
        enum_<ins_camera::CaptureSettings::SettingsType>("SettingsType")
            .value("CaptureSettins_Contrast", ins_camera::CaptureSettings::SettingsType::CaptureSettings_Contrast)
            .value("CaptureSettins_Saturation", ins_camera::CaptureSettings::SettingsType::CaptureSettings_Saturation)
            .value("CaptureSettins_Brightness", ins_camera::CaptureSettings::SettingsType::CaptureSettings_Brightness)
            .value("CaptureSettins_Sharpness", ins_camera::CaptureSettings::SettingsType::CaptureSettings_Sharpness)
            .value("CaptureSettins_WhiteBalance", ins_camera::CaptureSettings::SettingsType::CaptureSettings_WhiteBalance)
            ;// .export_values();
        class_<std::vector<ins_camera::CaptureSettings::SettingsType>>("vector_SettingsType")
            .def(vector_indexing_suite<std::vector<ins_camera::CaptureSettings::SettingsType>>());
    }
    class_<ins_camera::RecordParams>("RecordParams")
        .add_property("resolution", &ins_camera::RecordParams::resolution)
        .add_property("bitrate", &ins_camera::RecordParams::bitrate);
    class_<ins_camera::LiveStreamParam>("LiveStreamParam")
        .add_property("enable_audio", &ins_camera::LiveStreamParam::enable_audio)
        .add_property("enable_video", &ins_camera::LiveStreamParam::enable_video)
        .add_property("audio_samplerate", &ins_camera::LiveStreamParam::audio_samplerate)
        .add_property("audio_bitrate", &ins_camera::LiveStreamParam::audio_bitrate)
        .add_property("video_bitrate", &ins_camera::LiveStreamParam::video_bitrate)
        .add_property("video_resolution", &ins_camera::LiveStreamParam::video_resolution)
        .add_property("lrv_video_bitrate", &ins_camera::LiveStreamParam::lrv_video_bitrate)
        .add_property("lrv_video_resolution", &ins_camera::LiveStreamParam::lrv_video_resulution)
        .add_property("enable_gyro", &ins_camera::LiveStreamParam::enable_gyro)
        .add_property("using_lrv", &ins_camera::LiveStreamParam::using_lrv);
    class_<ins_camera::TimelapseParam>("TimelapseParam")
        .add_property("mode", &ins_camera::TimelapseParam::mode)
        .add_property("duration", &ins_camera::TimelapseParam::duration)
        .add_property("lapse_time", &ins_camera::TimelapseParam::lapseTime)
        .add_property("accelerate_fequency", &ins_camera::TimelapseParam::accelerate_fequency);

    class_<TestStreamDelegate>("TestStreamDelegate");
    

}
// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します
