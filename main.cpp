
#include "encodecvideo.h"
#include <map>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <chrono>
// #include "mem2vid.h"

// EncodecVideo pencodevideo;
//  videoencoder encoder;
#include <opencv2/opencv.hpp>
#include "muxingvideo.h"


VideoEncoder pvideoencoder;
MuxingVideo muxingvideo;


void laodfilelist()
{
    std::map <std::string ,int> filenamelist;
    // std::cout<<m_videoFileName.toLocal8Bit().data()<<std::endl;
	DIR *pDir;
    struct dirent* ptr;
    std::string path = "./image";
    if(!(pDir = opendir("./image")))
    {
        std::cout<<"Folder doesn't Exist!"<<std::endl;
        return;
    }
    while((ptr = readdir(pDir))!=0) {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0)
        {
            std::string name(ptr->d_name);
            int pos=name.find_last_of('.');
            std::string filename = name.substr(0,pos);
            filenamelist[filename] = 0;
   
    	}
    }
    closedir(pDir);
    for (auto&it:filenamelist)
    {
        std::string  color_filename =  path + "/" + it.first + ".jpg";
        cv::Mat tmpcolor = cv::imread(color_filename.c_str());
        // std::cout << color_filename << "***** YUV420P 00" << std::endl;
        if(tmpcolor.empty())
        {
            
            return;
        }
        else
        {

            // cv::Mat reslutimg;
            // //  cv::cvtColor(tmpcolor, reslutimg, CV_BGR2YUV_YV12);
            // cv::cvtColor(tmpcolor, reslutimg, cv::COLOR_BGR2ARGB);
            // int size = reslutimg.cols * reslutimg.rows * reslutimg.channels();
            // pvideoencoder.PutFrame(reslutimg.data,size);
            int size = tmpcolor.cols * tmpcolor.rows * tmpcolor.channels();
            pvideoencoder.PutFrame(tmpcolor.data,size);
            // index ++;

        }

    }
    


}

void writefile(uint8_t* data, uint32_t size)
{
    muxingvideo.WriteFrame(data,size);
}
#if 1
int main(void)
{

  
    // char out_filename[128] = "./test.mp4";

    StreamInfo streaminfo;
    streaminfo.StreamType = 0;
    streaminfo.gop = 60;
    FrameInfo frameinfo;
    frameinfo.width = 1920;
    frameinfo.height = 1080;
    frameinfo.format = 0;
    frameinfo.fps = 30;

    // pvideoencoder = new VideoEncoder();

    // fp_output = fopen("./test.h264", "w+b");
    // if (nullptr == fp_output)
    // {
    //    std::cout << "++++++++++++++++++++++++++++++++++++++++++\r\n";
    // }
    for (size_t i = 0; i < 10; i++)
    {
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
        std::string out_filename = "./test_";
        auto fileno = std::to_string(i);
        out_filename += fileno;
        out_filename += ".mp4";
        SpsHeader sps_header;
        pvideoencoder.Init(frameinfo,streaminfo,&writefile ,&sps_header);
        // init_rtmp_streamer(out_filename,nullptr,0);
    // init_rtmp_streamer(out_filename,sps_header.data,sps_header.size);
        muxingvideo.InitSaveStreamer(out_filename.c_str(),sps_header.data,sps_header.size,frameinfo);

        laodfilelist();

        pvideoencoder.Release();

        muxingvideo.finished();


        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
            std::cout << "eccode time ============================ " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;
    
    }
    
    


    return EXIT_SUCCESS;
}
#else



int main()
{
    const uint32_t FRAMES_PER_SECOND = 30;
    const uint32_t WIDTH = 1920;
    const uint32_t HEIGHT = 1080;

    video_param_t params = {.size_x=WIDTH, .size_y=HEIGHT, .frames_per_second=FRAMES_PER_SECOND, .bitrate=40000};
    int ret = video_start("./test", params); // initialize video with chosen parameters
    if (ret != EXIT_SUCCESS) {
        fprintf(stderr, "failed to initialize video\n");

        return EXIT_FAILURE;
    }


    laodfilelist();

    video_finish(); // finalize the video creation, perform cleanup
    return 0;
}
#endif



