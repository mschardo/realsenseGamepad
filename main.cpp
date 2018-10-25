#include <thread>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <librealsense/rs.hpp>

uint16_t* filterData(uint16_t* data, int width, int height, uint16_t range) {
  static uint16_t filtered[9999999];
  int i;
  uint16_t min = 65535;
  uint16_t minEsquerda = 65535;
  uint16_t minDireita = 65535;
  int meiota = 314;
  // uint16_t max, min;
  // max & 0000000000000000;
  // min | 1111111111111111;

  for (i=0;i<(width*height);i++){
    //linha no quadrante esquerdo
    if (!((data[i]/meiota)%2)) {
      if (data[i] > 0 && data[i] < minEsquerda) {
        minEsquerda = data[i];
    //linha no quadrante direito
      }
    } else {
      if (data[i] > 0 && data[i] < minDireita) {
        minDireita = data[i];
      }
    }
    //if (data[i] > 0 && data[i] < min) {
    //  min = data[i];
   // }
  }

  // min = 500;
  // std::cout << "Min: " << min << std::endl;

  uint16_t maxDireita = minDireita + range;
  uint16_t maxEsquerda = minEsquerda + range;

  for (i=0;i<(width*height);i++) {
    if (!((data[i]/meiota)%2)) {
    // Na esquerda
      if (data[i] >= maxEsquerda) {
        filtered[i] = 0;
      } else if (data[i] <= minEsquerda) {
        filtered[i] = 0;
      } else {
        filtered[i] = 65535;
      }
    } else {
    // Na direita
      if (data[i] >= maxDireita) {
        filtered[i] = 0;
      } else if (data[i] <= minDireita) {
        filtered[i] = 0;
      } else {
        filtered[i] = 65535;
      }
    }
  }

  return filtered;

}

int getAxis(cv::Mat data, int filter) {
  int i, j;
  int left = 0;
  int right = 0;
  int nLeft = 0;
  int nRight = 0;
  int angle;
  float meiota = data.cols/2;

  // uint16_t max, min;
  // max & 0000000000000000;
  // min | 1111111111111111;

  for(i = 0; i < (data.rows - filter); i++) {
    // uchar* Mi = data.ptr(i);
    if (i > filter) {
      for(j = 0; j < (data.cols - filter); j++) {
        // std::cout << Mi[j] << std::endl;
        if (j > filter) {
          if (data.at<int>(i,j) != 0) {
            if (j <= meiota) { // está na esquerda
              nLeft = nLeft + 1;
              left = left + i;
            } else { // está na direita
              nRight = nRight + 1;;
              right = right + i;
            }
          }
        }
      }
    }
  }

  // for (i=0;i<(width*height);i++) {
  //   if (data[i] > 0) {
  //     if ((i%width < meiota)) { // está na esquerda
  //       nLeft ++;
  //       left += i/width;
  //     } else { // está na direita
  //       nRight ++;
  //       right += i/width;
  //     }
  //   }
  // }

  double mLeft = left/(nLeft+1);
  double mRight = right/(nRight+1);

  // std::cout << "Left " << " " << left << " " << nLeft << " " << mLeft << std::endl ;
  // std::cout << "Right " << " " << right << " " << nRight << " " << mRight << std::endl ;

  angle = (int) ((mLeft - mRight) * (127 - (-128)) / (30 - (-30)));
  if (angle < -128) { return -128; }
  else if (angle > 127) { return 127; }
  else { return angle; }
}

int main(int argc, char** argv)
{
    rs::context ctx;
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;
    rs::device * dev = ctx.get_device(0);

    const uint16_t meter = static_cast<uint16_t>(1.0f/dev->get_depth_scale());

    dev->enable_stream(rs::stream::depth, 0, 0, rs::format::z16, 60);
    // rs::device is NOT designed to be thread safe, so you should not currently make calls
    // against it from your callback thread. If you need access to information like 
    // intrinsics/extrinsics, formats, etc., capture them ahead of time
    rs::intrinsics depth_intrin, color_intrin;
    rs::format depth_format, color_format;
    depth_intrin = dev->get_stream_intrinsics(rs::stream::depth);
    depth_format = dev->get_stream_format(rs::stream::depth);

    // Set callbacks prior to calling start()
    cv::Mat frameGray;
    cv::Mat frameGrayNrm;
    auto depth_callback = [depth_intrin, depth_format, &frameGray, &frameGrayNrm](rs::frame f)
    {
      cv::Mat frame(cv::Size(depth_intrin.width,depth_intrin.height), CV_16UC1, filterData((uint16_t*)f.get_data(),depth_intrin.width,depth_intrin.height,150));
      frame.convertTo(frameGray, CV_8U, 1. / 64);
      int i;
      for (i=0;i<(depth_intrin.width*depth_intrin.height);i++) {

      }
      // cv::normalize(frameGray, frameGrayNrm, 0, 256, cv::NORM_MINMAX);
      // const int channelMap[] = { 0,0, 0,1, 0,2 };
      // cv::mixChannels(&frame, 1, &frameColor, 1, channelMap, 3);

      // int count = 0;
      // cv::MatConstIterator_<int> it = frame.begin<int>(), it_end = frame.end<int>();
      // for(; it != it_end; ++it) {
      //   std::cout << *it << std::endl;
      // }

    };
    dev->set_frame_callback(rs::stream::depth, depth_callback);

    // Between start() and stop(), you will receive calls to the specified callbacks from background threads
    dev->start();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    int frames = 0;
    int axisHistory[5] = {0,0,0,0,0};
    int buffer;

    while (1) {
      cv::Mat rgb;
      // cv::applyColorMap(frameGray, rgb, cv::COLORMAP_COOL);
      cv::morphologyEx(frameGray, rgb, cv::MORPH_OPEN, cv::Mat());
      cv::morphologyEx(rgb, rgb, cv::MORPH_CLOSE, cv::Mat());
      // cv::morphologyEx(rgb, rgb, cv::MORPH_GRADIENT, cv::Mat());
      cv::imshow("this is you, smile! :)", rgb);

      if (frames == 6) {
        int i, soma = 0;
        double value;
        for (i=0;i<frames;i++){
          soma += axisHistory[i];
        }
        value = soma/frames;
        frames = 0;
        std::cout << value << std::endl;
        
      } else {
        buffer = getAxis(rgb, 30);
        if (buffer == 0) {
          for (int jota =0;jota<6;jota++) {
            axisHistory[jota] = 0;
          }
        } else {
          axisHistory[frames] = buffer;
        }
        frames += 1;
      }
      cv::waitKey(1);
    }
    dev->stop();
    return 0;
}
