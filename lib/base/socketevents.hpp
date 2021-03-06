/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef SOCKETEVENTS_H
#define SOCKETEVENTS_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"

namespace icinga
{

/**
 * Socket event interface
 *
 * @ingroup base
 */
class I2_BASE_API SocketEvents
{
public:
	~SocketEvents(void);

	virtual void OnEvent(int revents);

	void Unregister(void);

	void ChangeEvents(int events);

	bool IsHandlingEvents(void) const;

protected:
	SocketEvents(const Socket::Ptr& socket, Object *lifesupportObject);

private:
	SOCKET m_FD;
	bool m_Events;

	static void InitializeThread(void);
	static void ThreadProc(void);

	static void WakeUpThread(bool wait = false);

	int GetPollEvents(void) const;

	void Register(Object *lifesupportObject);
};

}

#endif /* SOCKETEVENTS_H */
