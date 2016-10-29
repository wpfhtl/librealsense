// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

///////////////////
// cpp-headless  //
///////////////////

// This sample captures 30 frames and writes the last frame to disk.
// It can be useful for debugging an embedded system with no display.

#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "example.hpp"

#include <cstdio>
#include <stdint.h>
#include <vector>
#include <limits>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"
#include <sstream>

int main() try
{
    rs::log_to_console(RS_LOG_SEVERITY_WARN);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    auto list = ctx.query_devices();
    printf("There are %llu connected RealSense devices.\n", list.size());
    if(list.size() == 0) return EXIT_FAILURE;

    auto dev = list[0];
    printf("\nUsing device 0, an %s\n", dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME));
    printf("    Serial number: %s\n", dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
    printf("    Firmware version: %s\n", dev.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION));

    rs::util::config config;
    config.enable_all(rs::preset::best_quality);
    auto stream = config.open(dev);

    rs::util::syncer sync;
    /* activate video streaming */
    stream.start(sync);

    /* Capture 30 frames to give autoexposure, etc. a chance to settle */
    for (auto i = 0; i < 30; ++i) sync.wait_for_frames();

    /* Retrieve data from all the enabled streams */
    std::map<rs_stream, rs::frame> frames_by_stream;
    for (auto&& frame : sync.wait_for_frames())
    {
        auto stream_type = frame.get_stream_type();
        frames_by_stream[stream_type] = std::move(frame);
    }

    /* Store captured frames into current directory */
    for (auto&& kvp : frames_by_stream)
    {
        auto stream_type = kvp.first;
        auto& frame = kvp.second;

        std::stringstream ss;
        ss << "cpp-headless-output-" << stream_type << ".png";

        std::cout << "Writing " << ss.str().data() << ", " << frame.get_width() << " x " << frame.get_height() << " pixels"   << std::endl;

        auto pixels = frame.get_data();
        auto bpp = frame.get_bytes_per_pixel();
        std::vector<uint8_t> coloredDepth;

        // Create nice color image from depth data
        if (stream_type == RS_STREAM_DEPTH)
        {
            /* Transform Depth range map into color map */
            auto& depth = frames_by_stream[RS_STREAM_DEPTH];
            const auto depth_size = depth.get_width() * depth.get_height() * 3;
            coloredDepth.resize(depth_size);

            /* Encode depth data into color image */
            make_depth_histogram(coloredDepth.data(), 
                static_cast<const uint16_t*>(depth.get_data()), 
                depth.get_width(), depth.get_height());

            pixels = coloredDepth.data();
            bpp = 3;
        }

        stbi_write_png(ss.str().data(),
            frame.get_width(), frame.get_height(),
            bpp,
            pixels,
            frame.get_width() * bpp );
    }

    frames_by_stream.clear();

    printf("wrote frames to current working directory.\n");
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
