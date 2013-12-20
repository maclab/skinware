/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ROSSKIN_H
#define ROSSKIN_H

#include <ros/ros.h>
#include <skin.h>

namespace rosskin
{
class Connector
{
public:
	/* callback to be called if periodic. */
	typedef void (*callback_func)(rosskin::Connector &, void *);

	skin_object skin;

	/*
	 * load the skin and start acquisition.
	 *
	 * If period is 0, then the read is sporadic.  If non zero, then asks the server for
	 * a publisher with the desired period.  Note that the skin may be working at a
	 * larger period than specified here.  In such a case, the publisher will be capped
	 * at the period of the acquisition, and therefore the messages received may not
	 * necessarily arrive at this given period.
	 *
	 * If periodic, a callback would be registered to be called everytime a message is
	 * received.  This callback is passed this object (to access `skin`) as well as
	 * a user defined `data` pointer.
	 *
	 * note: if you wait for ROS to shutdown with a signal, then it would not be possible
	 * to cleanly detach from the server in `unload` until ROS implements an on_shutdown
	 * callback.  Therefore, make sure you handle the signals yourself and call `unload`
	 * before ros::shutdown()
	 */
	int load(ros::NodeHandle &n, uint64_t period = 0, callback_func cb = NULL, void *data = NULL);
	void unload();

	/*
	 * request a sporadic read
	 *
	 * Works only if loaded in sporadic mode.  In this case, the function blocks until
	 * a response is received (or failed).  Once done, the `skin` can be accessed to
	 * retrieve the sensor values.
	 */
	int request();

	/* pause/resume acquisition for periodic case */
	void pause();
	void resume();

	Connector(): callback(NULL), user_data(NULL), period(0), started(false), paused(false), internal(NULL) {}
	~Connector() { unload(); }

	callback_func callback;
	void *user_data;
private:
	uint64_t period;
	uint32_t id;
	bool started;
	bool paused;
	ros::ServiceClient cmd_srv;
	ros::Subscriber sub;
	ros::ServiceClient req_srv;
	void *internal;
};
}

#endif
