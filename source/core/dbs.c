#include "dbs.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDENT_SIZE 2

// Data
static DBusConnection* dbs_connection = NULL;

// Helper Functions
static void dbs_expandIter(DBusMessageIter* iter, char* buffer, int bufferSize, int indent);
static void dbs_writeToBuffer(const char* str, char* buffer, int bufferSize, int indent);

// Checks if the DBus utils are initialized
int dbs_init()
{
	DBusError err;
	dbus_error_init(&err);

	//connect to system bus
	dbs_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if(dbus_error_is_set(&err)) {
		printf("dbus_init: Connection error [%s]\n", err.message);
		dbus_error_free(&err);
		
		dbs_close();
		return 1;
	}
	
	return 0;
}

// Checks if the DBus utils are initialized
char dbs_isInit()
{
	return (dbs_connection != NULL);
}

// Makes a simple method call with no arguments or reply
void dbs_simpleMethodCall(const char* objectPath, const char* busName, const char* interfaceName, const char* methodName)
{
	if(objectPath != NULL && busName != NULL && interfaceName != NULL && methodName != NULL) {
		if(dbs_connection != NULL) {
			DBusMessage* msg = dbus_message_new_method_call(busName, objectPath, interfaceName, methodName);
			if(msg != NULL) {
				dbus_connection_send(dbs_connection, msg, NULL);
				dbus_connection_flush(dbs_connection);
				dbus_message_unref(msg);
			} else fprintf(stderr, "dbus_simpleMethodCall: Failed to create message\n");
		} else fprintf(stderr, "dbus_simpleMethodCall: Connection not initialized\n");
	} else fprintf(stderr, "dbus_simpleMethodCall: Invalid Parameters\n");
}

// Makes a complex method call with arguments and expects a reply
void dbs_complexMethodCall(char* replyBuffer, int replyBufferSize, const char* objectPath, const char* busName, const char* interfaceName, const char* methodName, const char* arg0, const char* arg1, const char* arg2)
{
	DBusMessageIter args;
	DBusMessage* msg;
	replyBuffer[0] = 0;
	if(objectPath != NULL && busName != NULL && interfaceName != NULL && methodName != NULL && replyBuffer != NULL) {
		if(dbs_connection != NULL) {
			
			//create message
			msg = dbus_message_new_method_call(busName, objectPath, interfaceName, methodName);
			if(msg != NULL) {
				
				//append arguments
				if(arg0 != NULL) {
					dbus_message_iter_init_append(msg, &args);
					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg0);
					if(arg1 != NULL) dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg1);
					if(arg2 != NULL) dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg2);
				}
				
				//send message and get a handle for a reply
				DBusPendingCall* pending;
				if(dbus_connection_send_with_reply(dbs_connection, msg, &pending, -1)) {
					dbus_connection_flush(dbs_connection);
					dbus_message_unref(msg);

					//wait for and get reply
					dbus_pending_call_block(pending);
					msg = dbus_pending_call_steal_reply(pending);
					dbus_pending_call_unref(pending);
					if(msg != NULL) {
						if(dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_ERROR) {
						
							// read the reply parameters
							if(dbus_message_iter_init(msg, &args)) {
								dbs_expandIter(&args, replyBuffer, replyBufferSize, 0);
							}
							
						} else fprintf(stderr, "dbus_complexMethodCall: Call returned error [%s]\n", dbus_message_get_error_name(msg));
						dbus_message_unref(msg);
						
					} else fprintf(stderr, "dbus_complexMethodCall: Failed to get message reply\n");
				} else fprintf(stderr, "dbus_complexMethodCall: Failed to send message\n");
			} else fprintf(stderr, "dbus_complexMethodCall: Failed to create message\n");
		} else fprintf(stderr, "dbus_complexMethodCall: Connection not initialized\n");
	} else fprintf(stderr, "dbus_complexMethodCall: Invalid Parameters\n");
}

// Cleans up the DBus utils
int dbs_close()
{
	if(dbs_connection != NULL) {
		//dbus_connection_close(dbs_connection);
		dbs_connection = NULL;
	}
	return 0;
}

// Helper Functions
static void dbs_expandIter(DBusMessageIter* iter, char* buffer, int bufferSize, int indent) {
	char temp[128];
	
	int argType = dbus_message_iter_get_arg_type(iter);
	if(argType == DBUS_TYPE_BYTE) {
		unsigned char val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "byte:%d", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_BOOLEAN) {
		dbus_bool_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "boolean:%s", val ? "true" : "false");
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_INT16) {
		dbus_int16_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "int16:%d", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_UINT16) {
		dbus_uint16_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "uint16:%u", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_INT32) {
		dbus_int32_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "int32:%d", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_UINT32) {
		dbus_uint32_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "uint32:%u", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_INT64) {
		dbus_int64_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "int64:%I64d", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_UINT64) {
		dbus_uint64_t val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "uint64:%I64u", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_DOUBLE) {
		double val;
		dbus_message_iter_get_basic(iter, &val);
		sprintf(temp, "double:%g", val);
		dbs_writeToBuffer(temp, buffer, bufferSize, indent);
	
	} else if(argType == DBUS_TYPE_STRING) {
		char* val = NULL;
		dbus_message_iter_get_basic(iter, &val);
		dbs_writeToBuffer("string:", buffer, bufferSize, indent);
		dbs_writeToBuffer(val, buffer, bufferSize, 0);
	
	} else if(argType == DBUS_TYPE_OBJECT_PATH) {
		char* val = NULL;
		dbus_message_iter_get_basic(iter, &val);
		dbs_writeToBuffer("objectpath:", buffer, bufferSize, indent);
		dbs_writeToBuffer(val, buffer, bufferSize, 0);
	
	} else if(argType == DBUS_TYPE_SIGNATURE) {
		char* val = NULL;
		dbus_message_iter_get_basic(iter, &val);
		dbs_writeToBuffer("signature:", buffer, bufferSize, indent);
		dbs_writeToBuffer(val, buffer, bufferSize, 0);
	
	} else if(argType == DBUS_TYPE_VARIANT) {
		DBusMessageIter sub;
		dbus_message_iter_recurse(iter, &sub);
		
		dbs_writeToBuffer("variant:", buffer, bufferSize, indent);
		dbs_expandIter(&sub, buffer, bufferSize, 0);
		
	} else if(argType == DBUS_TYPE_ARRAY) {
		DBusMessageIter sub;
		dbus_message_iter_recurse(iter, &sub);
		
		dbs_writeToBuffer("array:\n", buffer, bufferSize, indent);
		dbs_expandIter(&sub, buffer, bufferSize, indent+INDENT_SIZE);
		
	} else if(argType == DBUS_TYPE_STRUCT) {
		DBusMessageIter sub;
		dbus_message_iter_recurse(iter, &sub);
		
		dbs_writeToBuffer("struct:\n", buffer, bufferSize, indent);
		dbs_expandIter(&sub, buffer, bufferSize, indent+INDENT_SIZE);
		
	} else if(argType == DBUS_TYPE_DICT_ENTRY) {
		DBusMessageIter sub;
		dbus_message_iter_recurse (iter, &sub);

		dbs_writeToBuffer("dictentry:\n", buffer, bufferSize, indent);
		dbs_expandIter(&sub, buffer, bufferSize, indent+INDENT_SIZE);
		
	} else if(argType == DBUS_TYPE_UNIX_FD) {
		//not yet supported
	}
	
	if(dbus_message_iter_next(iter)) {
		if(argType != DBUS_TYPE_INVALID) dbs_writeToBuffer("\n", buffer, bufferSize, 0);
		dbs_expandIter(iter, buffer, bufferSize, indent);
	}
}
static void dbs_writeToBuffer(const char* str, char* buffer, int bufferSize, int indent) {
	int i;
	int bufferLen = strlen(buffer);
	int writeLen = strlen(str) + indent;
	if(bufferLen + writeLen < (bufferSize-1)) {
		buffer += bufferLen;
		for(i=0; i<indent; i++) buffer[i] = ' ';
		strcpy(buffer+indent, str);
		
	} else {
		buffer += bufferLen;
		for(i=0; i<bufferSize-bufferLen; i++) buffer[i] = ' ';
		buffer[bufferSize-1] = 0;
	}		
}
