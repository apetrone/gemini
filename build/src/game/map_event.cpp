// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include "typedefs.h"
#include "log.h"
#include "map_event.hpp"


EventBasedMap::EventBasedMap()
{
	total_events = 0;
	events = 0;
} // EventBasedMap

EventBasedMap::~EventBasedMap()
{
	if (total_events > 0)
	{
		DESTROY_ARRAY(MapEvent, events, total_events);
		total_events = 0;
	}
} // ~EventBasedMap


util::ConfigLoadStatus map_event_loader( const Json::Value & root, void * data )
{
	EventBasedMap * map = (EventBasedMap*)data;
	if (!map)
	{
		return util::ConfigLoad_Failure;
	}



	Json::Value events = root["events"];
	map->total_events = events.size();
	map->events = CREATE_ARRAY(MapEvent, map->total_events);

	Json::ValueIterator event_iterator = events.begin();
	for( int event_id = 0; event_id < map->total_events; ++event_id, ++event_iterator )
	{
		LOGV( "reading event_id: %i\n", event_id );
		Json::Value key = event_iterator.key();
		Json::Value value = (*event_iterator);

		MapEvent * mapevent = &map->events[ event_id ];

		Json::Value time_value = value["time"];
		mapevent->time_value = time_value.asFloat();
		LOGV( "event time: %g\n", mapevent->time_value );

		Json::Value event_name = value["event"];
		mapevent->name = event_name.asString().c_str();
		LOGV( "name: %s\n", mapevent->name() );

		Json::Value position = value["pos"];
		mapevent->pos = position.asFloat();
		LOGV( "pos: %g\n", mapevent->pos );


		//value.asFloat()
	}

	// time, event, pos

	LOGV("loaded %i total map events\n", map->total_events);


	return util::ConfigLoad_Success;
}