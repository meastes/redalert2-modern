// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NATIVE glue appended to the queue unit (lanes Y3, YW2; lane P64 retired the drafts upstream now
// matches: Execute_DoList, Queue_AI_Multiplayer, Wait_For_Players, Draw_Sync_Bars, the event
// queue's Next, Game::LogFrameCRC). Left: the LAN tests' frame CRC log and FUN_0064BDD0, which
// ra2-decomp has not matched. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
#include <stdlib.h>
extern int DAT_00AC50FC[64];

// The frame's game CRC in the log for the LAN tests (lane YW2; fixups.json, Queue_AI_Multiplayer):
// "crc <frame> <value>" with RA2_NATIVE_CRC_LOG set, as the RA2 game's (native/link/native_event_data.cpp).
void Yr_Native_Log_Frame_CRC(int frame, unsigned int crc)
{
	static int enabled = -1;
	if (enabled < 0) enabled = getenv("RA2_NATIVE_CRC_LOG") != NULL;
	if (enabled) {
		fprintf(stderr, "crc %d %08x\n", frame, crc);
	}
}

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane YW2, LAN): FUN_0064BDD0, YR 0x0064BDD0 (1,455 bytes, unmatched in
// ra2-decomp): Extract_Compressed_Events, the receive side of FUN_0064B660 (the compressed events of
// COMM_PROTOCOL_MULTI_E_COMP, the network game's protocol): a packet's events back into the DoList.
// From gamemd.exe's code, which is the RA2 game's Extract_Compressed_Events (RA2 0x006282D0, the same
// 414 instructions; its native draft in src/recovered/queue.cpp, played in RA2's LAN games, with its
// fixes: the packet's end ends the loop, malformed records stop the decoding), in YR's names. A
// megamission with a unit list stores the list by the sender's house and slot (gamemd.exe 0x00AFA468,
// counts DAT_00AC50FC) for later megamissions that name only the slot (FUN_0064B4D0's cache).
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
static xTargetClass Yr_Received_Unit_Lists[8][8][128];

int __fastcall FUN_0064BDD0(void * buf, int bufsize)
{
	int pos = 0;
	int leftover = bufsize;
	EventClass * event;
	int count = 0;
	int datasize;
	EventClass eventdata;
	unsigned char numunits = 0;
	int house;
	int slot;
	int units;
	xTargetClass * list;
	char * src;

	memset(&eventdata, 0, sizeof(EventClass));
	house = ((EventClass *)buf)->ID;
	datasize = 13;
	event = (EventClass *)buf;

	while (leftover >= datasize + 1) {
		if (event->Type == EventClass::FRAMESYNC) {
			pos += datasize + 1;
			leftover -= datasize + 1;
			event = (EventClass *)(((char *)buf) + pos);
			datasize = 13;
			continue;
		}
		if (event->Type == EventClass::FRAMEINFO) {
			eventdata.ID = event->ID;
			eventdata.Frame = event->Frame;
			pos += 6;
			leftover -= 6;
		} else if (event->Type == EventClass::MEGAMISSION) {
			numunits = *(((unsigned char *)buf) + pos + 1);
			pos += 1;
			leftover -= 1;
		}
		memset(&eventdata.Data, 0, sizeof(eventdata.Data));
		eventdata.Type = event->Type;
		if (eventdata.Type >= EventClass::LAST_EVENT) {
			DebugString("Extract_Compressed_Events: bad event type %d at %d of %d\n", eventdata.Type, pos, bufsize);
			return(count);
		}
		datasize = EventClass::EventLength[eventdata.Type];
		if (pos + 1 + datasize > bufsize) {
			DebugString("Extract_Compressed_Events: event %d runs past the packet (%d of %d)\n", eventdata.Type, pos, bufsize);
			return(count);
		}

		switch (eventdata.Type) {
			case EventClass::RESPONSE_TIME:
				memcpy(&eventdata.Data.FrameInfo.Delay, ((char *)buf) + pos + 1, datasize);
				break;

			case EventClass::ADDPLAYER:
				memcpy(&eventdata.Data.Variable.Size, ((char *)buf) + pos + 1, datasize);
				if ((int)eventdata.Data.Variable.Size < 0 || pos + 1 + datasize + (int)eventdata.Data.Variable.Size > bufsize) {
					return(count);
				}
				eventdata.Data.Variable.Pointer = new char[eventdata.Data.Variable.Size];
				memcpy(eventdata.Data.Variable.Pointer, ((char *)buf) + pos + 1 + datasize, eventdata.Data.Variable.Size);
				pos += eventdata.Data.Variable.Size;
				leftover -= eventdata.Data.Variable.Size;
				break;

			case EventClass::MEGAMISSION:
				memcpy(&eventdata.Data, ((char *)buf) + pos + 1, datasize);
				slot = eventdata.Data.MegaMission.Slot;
				if (numunits > 1) {
					pos += datasize + 1;
					leftover -= datasize + 1;
					datasize = sizeof(xTargetClass);
					DebugString("Decoding new MEGAMISSION\n");
					if (house < 0 || house >= 8 || slot < 0 || slot >= 8 || numunits > 128 || pos + (numunits - 1) * datasize > bufsize) {
						return(count);
					}
					Yr_Received_Unit_Lists[house][slot][0] = eventdata.Data.MegaMission.Whom;
					units = 1;
					DAT_00AC50FC[house * 8 + slot] = units;
					src = ((char *)buf) + pos;
					list = &Yr_Received_Unit_Lists[house][slot][1];
					while (numunits) {
						if (!DoList.Add(eventdata)) {
							return(-1);
						}
						count++;
						*list = *(xTargetClass *)src;
						eventdata.Data.MegaMission.Whom = *list++;
						numunits--;
						units++;
						DAT_00AC50FC[house * 8 + slot] = units;
						if (numunits == 1) {
							datasize -= 1;
							DebugString("Got new unit list for player %d with %d units in slot %d\n", house, units, slot);
							break;
						}
						pos += datasize;
						src += datasize;
						leftover -= datasize;
					}
				} else if (numunits == 0) {
					pos += datasize + 1;
					leftover -= datasize + 1;
					datasize = sizeof(xTargetClass);
					units = 1;
					if (house < 0 || house >= 8 || slot < 0 || slot >= 8 || DAT_00AC50FC[house * 8 + slot] > 128) {
						return(count);
					}
					numunits = DAT_00AC50FC[house * 8 + slot];
					eventdata.Data.MegaMission.Whom = Yr_Received_Unit_Lists[house][slot][0];
					list = &Yr_Received_Unit_Lists[house][slot][1];
					while (numunits) {
						if (!DoList.Add(eventdata)) {
							return(-1);
						}
						count++;
						numunits--;
						units++;
						eventdata.Data.MegaMission.Whom = *list++;
						if (numunits == 1) {
							datasize = -1;
							DebugString("Used old unit list in slot %d - %d bytes saved\n", slot, units * 5);
							break;
						}
					}
				}
				break;

			default:
				memcpy(&eventdata.Data, ((char *)buf) + pos + 1, datasize);
				break;
		}

		if (!DoList.Add(eventdata)) {
			if (eventdata.Type == EventClass::ADDPLAYER) {
				delete[] (char *)eventdata.Data.Variable.Pointer;
			}
			return(-1);
		}
		count++;
		pos += datasize + 1;
		leftover -= datasize + 1;
		if (leftover == 0) {
			return(count);
		}
		event = (EventClass *)(((char *)buf) + pos);
		datasize = EventClass::EventLength[event->Type];
		if (event->Type == EventClass::MEGAMISSION) {
			datasize += 1;
		}
	}
	return(count);
}

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3): FUN_00652540 (YR 0x00652540), the event queue's Next for
// OutList: upstream matches it as the template QueueClass<EventClass,128>::Next, which the native
// build does not emit under this name (P64), so the forwarding stays.
// ---------------------------------------------------------------------------------------------
void __fastcall FUN_00652540(void * list)
{
	((decltype(&OutList))list)->Next();
}
