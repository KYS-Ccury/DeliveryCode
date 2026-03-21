#pragma once
// Protocol.h - Command IDs matching server Types.h exactly

// Common / Auth (100s)
#define CMD_SIGNUP          100
#define CMD_LOGIN           101
#define CMD_LOGOUT          102
#define CMD_GET_MY_INFO     104

// Rider (400s)
#define CMD_RIDER_ORDER_LIST     400   // dispatch list
#define CMD_RIDER_ACCEPT         401   // accept dispatch
#define CMD_RIDER_REJECT         402   // reject dispatch
#define CMD_RIDER_PICKUP_DONE    403   // pickup complete
#define CMD_RIDER_DELIVERY_DONE  404   // delivery complete
#define CMD_RIDER_MY_LIST        405   // my delivery history
#define CMD_RIDER_STATUS_UPDATE  406   // work status / dispatch on-off
#define CMD_RIDER_GPS            407   // GPS position send
#define CMD_RIDER_DISPATCH_PUSH  408   // server push: new dispatch

// Chat (600s)
#define CMD_CHAT_CREATE     600
#define CMD_CHAT_SEND       601
#define CMD_CHAT_HISTORY    602
#define CMD_CHAT_RECV_NTF   604        // server push: chat message

// Client type
#define CLIENT_TYPE_RIDER     3

// Server status codes
#define STATUS_SUCCESS      2000
#define STATUS_BAD_REQUEST  4000
#define STATUS_UNAUTHORIZED 4001
#define STATUS_NOT_FOUND    4004
#define STATUS_SERVER_ERROR 5000

// Legacy aliases
#define CMD_RIDER_DELIVERY_START  CMD_RIDER_PICKUP_DONE
#define PUSH_DISPATCH             CMD_RIDER_DISPATCH_PUSH
