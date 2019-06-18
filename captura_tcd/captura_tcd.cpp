#include "stdio.h"
#include <iostream>
#include <ctime>
#include <chrono>
#include <Windows.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio/videoio.hpp"

#include "camara_termica_xenics.h"
#include "librealsense2/rs.hpp"

#pragma warning(disable:4996)

// VARIABLE DEFINITION
#define CHESS 0;
#define SYM 1;
#define ASYM 2;

// FUNCTIONS
std::string exePath()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

std::string str2path(std::string s)
{
	std::replace(s.begin(), s.end(), '/', '\\');
	return s;
}


bool deleteDir(std::string path, bool outdir)
{
	path = str2path(path);
	if (outdir)
	{
		path += "\\output_images\\";
	}
	
	if (!dirExists(path))
	{
		return true;
	}
	else
	{
		if (path.substr(path.length() - 14) == "output_images\\")
		{
			std::string cpath = "rmdir /Q /S \"";
			cpath += path;
			cpath += "\"";
			system(cpath.c_str());
		}
		else if (path.substr(path.length() - 6) == "color\\")
		{
			std::string cpath = "rmdir /Q /S \"";
			cpath += path;
			cpath += "\"";
			system(cpath.c_str());

		}
		else if (path.substr(path.length() - 6) == "depth\\")
		{
			std::string cpath = "rmdir /Q /S \"";
			cpath += path;
			cpath += "\"";
			system(cpath.c_str());
		}
		else if (path.substr(path.length() - 7) == "thermo\\")
		{
			std::string cpath = "rmdir /Q /S \"";
			cpath += path;
			cpath += "\"";
			system(cpath.c_str());
		}

	}
	if (dirExists(path))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool createDir(std::string path)
{
	path = str2path(path);

	if (dirExists(path))
	{
		std::string cpath = "\\output_images\\color\\";
		std::string dpath = "\\output_images\\depth\\";
		std::string tpath = "\\output_images\\thermo\\";

		std::string coutput = path;
		std::string doutput = path;
		std::string toutput = path;
		coutput += cpath;
		doutput += dpath;
		toutput += tpath;

		if (!dirExists(coutput) || !dirExists(doutput) || !dirExists(toutput))
		{
			std::string cmdpath = "mkdir \"";
			cmdpath += coutput;
			cmdpath += "\"";
			system(cmdpath.c_str());

			cmdpath = "mkdir \"";
			cmdpath += doutput;
			cmdpath += "\"";
			system(cmdpath.c_str());

			cmdpath = "mkdir \"";
			cmdpath += toutput;
			cmdpath += "\"";
			system(cmdpath.c_str());

			return true;
		}

		if (dirExists(coutput) && dirExists(doutput) && dirExists(toutput))
		{
			return true;
		}
	}
	return false;
}

bool save_images(std::string path, std::string name, cv::Mat color, cv::Mat1w depth, cv::Mat thermo)
{
	path = str2path(path);

	std::string cpath = "\\output_images\\color\\";
	std::string dpath = "\\output_images\\depth\\";
	std::string tpath = "\\output_images\\thermo\\";

	if (color.empty() || depth.empty() || thermo.empty())
	{
		return false;
	}

	std::string output = path;
	output += cpath;
	output += "c_";
	output += name;
	output += ".png";
	if (!cv::imwrite(output, color))
	{
		return false;
	}

	output = path;
	output += dpath;
	output += "d_";
	output += name;
	output += ".png";
	if (!cv::imwrite(output, depth))
	{
		return false;
	}

	output = path;
	output += tpath;
	output += "t_";
	output += name;
	output += ".png";
	if (!cv::imwrite(output, thermo))
	{
		return false;
	}

	return true;
}

std::string get_time()
{
	time_t rawtime;
	char str_time[21];

	rawtime = time(NULL);
	struct tm tm = *localtime(&rawtime);
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	sprintf(str_time, "%d%d%d%s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, std::to_string(ms.count()).c_str());

	return str_time;
}

// MAIN ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	const std::string inputSettingsFile = argc > 1 ? argv[1] : "configuration.xml";
	
	// CONFIG
	std::string path_save = "";
	int delete_dir;
	int cd_resolution_mode = 0;
	int cd_imwidth = 1280;
	int cd_imheight = 720;
	int cd_fps = 30;

	bool intel_postprocess = true;
	int flip_mode = 0;
	int negative_mode = false;
	bool save_default_images = true;

	bool detect_circles = false;
	int board_width;
	int board_height;
	
	// READ CONFIG VALUES
	cv::FileStorage fs(inputSettingsFile, cv::FileStorage::READ); // Read the settings
	if (!fs.isOpened())
	{
		printf("ERROR: Could not open the configuration file: %s\n", inputSettingsFile.c_str());
		system("pause");
		return -1;
	}

	fs["PathSave"] >> path_save;
	fs["DeleteDir"] >> delete_dir;
	fs["IntelResolution_mode"] >> cd_resolution_mode;
	fs["IntelFPS"] >> cd_fps;
	fs["IntelPostprocess"] >> intel_postprocess;
	fs["FlipMode"] >> flip_mode;
	fs["NegativeMode"] >> negative_mode;
	fs["SaveDefaultImages"] >> save_default_images;
	fs["AsymCirlePaternDetector"] >> detect_circles;
	fs["BoardSize_Width"] >> board_width;
	fs["BoardSize_Height"] >> board_height;
	fs.release();

	// Check config params
	if (delete_dir < 0 || delete_dir > 1)
	{
		delete_dir = 0;
	}
	switch (cd_resolution_mode) // Check resolution
	{
	case 0:
		cd_imwidth = 1280;
		cd_imheight = 720;
		break;
	case 1:
		cd_imwidth = 640;
		cd_imheight = 480;
		break;
	case 2:
		cd_imwidth = 848;
		cd_imheight = 480;
		break;
	default:
		printf("Resolution mode not recogniced. Resolution set to 1280x720\n");
		cd_imwidth = 1280;
		cd_imheight = 720;
		break;
	}
	if (cd_fps < 0) // Check fps
	{
		printf("Invalid FPS.\n");
		system("pause");
		return -1;
	}
	if (flip_mode < 0 && flip_mode > 3) // Check flip mode
	{
		printf("Invalid flip mode.\n");
		system("pause");
		return -1;
	}
	if (negative_mode < 0 && negative_mode > 3)
	{
		printf("Invalid flip mode.\n");
		system("pause");
		return -1;
	}
	if (board_width <= 0 || board_height <= 0)
	{
		printf("Invalid negative mode.\n");
		system("pause");
		return -1;
	}

	cv::Size patternsize(board_height, board_width);
	
	// VARIABLES
	CamaraTermicaXenics t_cam;
	cv::Mat bkc_image;
	cv::Mat bkd_image;
	cv::Mat bkt_image;
	bool keep_alive = true;
	bool recording = false;


	// PREPARE APP
	// check if path exists
	if (path_save == "")
	{
		path_save = exePath();
	}

	if (delete_dir)
	{
		if (deleteDir(path_save, true))
		{
			printf("All images in output_images folder deleted.\n");
		}
		else
		{
			printf("Unable to delete output_images.\n");
		}
	}

	// create the directory for the images
	if (!createDir(path_save))
	{
		printf("Path does not exist or the app failed to create the output folder");
		system("pause");
		return -1;
	}

	// Configure intelrealsense
	rs2::pipeline pipe;
	rs2::config cfg;
	// Define transformations from and to Disparity domain
	rs2::decimation_filter dec;
	rs2::spatial_filter spat;
	rs2::temporal_filter temp;
	rs2::disparity_transform depth2disparity;
	rs2::disparity_transform disparity2depth(false);
	rs2::align align_to(RS2_STREAM_COLOR);
	rs2::frame depth_frame;
	cfg.enable_stream(RS2_STREAM_COLOR, cd_imwidth, cd_imheight, RS2_FORMAT_BGR8, cd_fps);
	cfg.enable_stream(RS2_STREAM_DEPTH, cd_imwidth, cd_imheight, RS2_FORMAT_Z16, cd_fps);

	// Start cameras
	auto prof = pipe.start(cfg);
	auto color_stream = prof.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
	auto depth_stream = prof.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
	t_cam.start();
	

	// MAIN LOOP
	printf
	(
		"- Press ESC to stop.\n"
		"- Press SPACE to capture.\n"
		"- Press R to START/STOP continuous frame recording.\n\n"
	);

	while (keep_alive)
	{
		cv::Mat c_image;
		cv::Mat d_image;
		cv::Mat d_image_cm;
		cv::Mat t_image;

		// Grab a frame
		t_cam.getFrame(t_image);

		rs2::frameset frames = pipe.wait_for_frames();
		frames = align_to.process(frames);

		rs2::frame color_frame = frames.get_color_frame();
		depth_frame = frames.get_depth_frame();
		rs2::frame filtered = depth_frame;

		if (intel_postprocess)
		{
			filtered = depth2disparity.process(filtered);
			filtered = spat.process(filtered);
			filtered = temp.process(filtered);
			filtered = disparity2depth.process(filtered);
			depth_frame = filtered;
		}

		int depth_width = depth_frame.as<rs2::video_frame>().get_width();
		int depth_height = depth_frame.as<rs2::video_frame>().get_height();
		int color_width = color_frame.as<rs2::video_frame>().get_width();
		int color_height = color_frame.as<rs2::video_frame>().get_height();

		// color image
		cv::Mat img(cv::Size(color_width, color_height), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
		// depth image
		cv::Mat depth(cv::Size(depth_width, depth_height), CV_16UC1, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);

		// colormap depth image
		rs2::colorizer color_map;
		frames.apply_filter(color_map);
		depth_frame = frames.get_depth_frame().apply_filter(color_map);
		cv::Mat depth_cm(cv::Size(depth_width, depth_height), CV_8UC3, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);


		// Save images variables
		c_image = img;
		d_image = depth;
		d_image_cm = depth_cm;

		// save backup images before modify if save_default_images is true
		if (save_default_images)
		{
			bkt_image = t_image.clone();
			bkc_image = c_image.clone();
			bkd_image = d_image.clone();
		}

		// Modify images
		// Flip
		cv::Mat t;
		switch (flip_mode)
		{
		case 1:
			cv::flip(t_image, t, 1);
			t_image = t.clone();
			break;
		case 2:
			cv::flip(c_image, img, 1);
			cv::flip(d_image, depth, 1);
			cv::flip(d_image_cm, depth_cm, 1);
			c_image = img.clone();
			d_image = depth.clone();
			d_image_cm = depth_cm.clone();
			break;
		case 3:
			cv::flip(t_image, t, 1);
			t_image = t;
			cv::flip(c_image, img, 1);
			cv::flip(d_image, depth, 1);
			cv::flip(d_image_cm, depth_cm, 1);
			c_image = img.clone();
			d_image = depth.clone();
			d_image_cm = depth_cm.clone();
			break;
		}
		// Negative
		switch (negative_mode)
		{
		case 1:
			cv::bitwise_not(t_image, t_image);
			break;
		case 2:
			cv::bitwise_not(c_image, c_image);
			break;
		case 3:
			cv::bitwise_not(t_image, t_image);
			cv::bitwise_not(c_image, c_image);
			break;
		}

		// save backup images before modify if save_default_images is false
		if (!save_default_images)
		{
			bkt_image = t_image.clone();
			bkc_image = c_image.clone();
			bkd_image = d_image.clone();
		}
		
		
		// Find circles
		bool t_circles_found;
		bool c_circles_found;
		std::vector<cv::Point2f> t_centers;
		std::vector<cv::Point2f> c_centers;
		if (detect_circles)
		{
			t_image.convertTo(t_image, CV_8UC1, 1 / 256.0);
			t_circles_found = cv::findCirclesGrid(t_image, patternsize, t_centers, cv::CALIB_CB_ASYMMETRIC_GRID);
			c_circles_found = cv::findCirclesGrid(c_image, patternsize, c_centers, cv::CALIB_CB_ASYMMETRIC_GRID);

			if (t_circles_found)
			{
				cv::drawChessboardCorners(t_image, patternsize, cv::Mat(t_centers), t_circles_found);
			}
			if (c_circles_found)
			{
				cv::drawChessboardCorners(c_image, patternsize, cv::Mat(c_centers), c_circles_found);
			}
		}


		if (t_image.empty() == false)
		{
			cv::namedWindow("Thermal camera", cv::WINDOW_AUTOSIZE);
			cv::imshow("Thermal camera", t_image);
		}
		if (c_image.empty() == false)
		{
			cv::namedWindow("Color camera", cv::WINDOW_AUTOSIZE);
			cv::imshow("Color camera", c_image);
		}
		if (d_image_cm.empty() == false)
		{
			cv::namedWindow("Depth camera", cv::WINDOW_AUTOSIZE);
			cv::imshow("Depth camera", d_image_cm);
		}

		char key = (char)cv::waitKey(50);

		if (key == 27) 
		{
			keep_alive = false;
		}

		if (key == ' ') 
		{
			std::string time = get_time();
			if (!save_images(path_save, time, bkc_image, bkd_image, bkt_image))
			{
				printf("Error while saveing the images, try again or restart the application.\n");
			} 
			else
			{
				printf("Images %s.png are saved.\n", time.c_str());
			}
		}
		if (key == 'r')
		{
			recording = !recording;
			if (recording)
			{
				printf("START recording frames.\n");
			}
			else
			{
				printf("STOP recording frames.\n");
			}
		}

		// record frames
		if (recording)
		{
			std::string time = get_time();
			if (!save_images(path_save, time, bkc_image, bkd_image, bkt_image))
			{
				printf("Error while saveing the images, try again or restart the application.\n");
			}
			else
			{
				printf("Images %s.png are saved.\n", time.c_str());
			}
		}
	}
	cv::destroyAllWindows();
	t_cam.close();

	system("pause");
	return 0;
}
