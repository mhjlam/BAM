//*
//* _STAR.HPP
//*
//* Contains:
//*              StarGate support structures
//*
//* The Greenleaf Comm++ Library
//*
//* Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
//*
//* MODIFICATIONS
//*
//*

#ifndef _STAR_HPP
#define _STAR_HPP

#define MAX_BOARDS 5
typedef enum { ACL_UNKNOWN = 0, ACLII, ACL16 } StarGate_Board_Type;

typedef struct {
    unsigned int global_command_word;
    unsigned int global_status_word;
    unsigned int global_service_request;
    unsigned int buffer_space_remaining;
    unsigned int board_type;
    unsigned int acl_control_program_version;
    unsigned int number_of_channels;
    unsigned int ccb_offset;
    unsigned int ccb_size;
    unsigned int global_command_word_2;
    unsigned int global_status_word_2;
    unsigned int comm_error_service_request;
    unsigned int input_buffer_service_request;
    unsigned int output_buffer_service_request;
    unsigned int modem_status_service_request;
    unsigned int channel_command_service_request;
} GLOBAL_CONTROL_BLOCK;

typedef struct {
    unsigned int baud_rate;
    unsigned int data_format;
    unsigned int line_protocol;
    unsigned int input_buffer_size;
    unsigned int output_buffer_size;
    unsigned int input_buffer_trigger_rate;
    unsigned int output_buffer_low_water_mark;
    unsigned int input_xon_xoff_characters;
    unsigned int input_buffer_high_water_mark;
    unsigned int input_buffer_low_water_mark;
    unsigned int channel_command;
    unsigned int channel_status;
    unsigned int input_buffer_start;
    unsigned int input_buffer_end;
    unsigned int output_buffer_start;
    unsigned int output_buffer_end;
    unsigned int next_character_to_input;
    unsigned int next_character_from_input;
    unsigned int next_character_to_output;
    unsigned int next_character_from_output;
    unsigned int communication_error_status;
    unsigned int bad_character_pointer;
    unsigned int modem_control_word;
    unsigned int modem_status_word;
    unsigned int blocking_status;
    unsigned int character_received_flag;
    unsigned int output_xon_xoff_characters;
    unsigned int channel_status_2;
} CHANNEL_CONTROL_BLOCK;


typedef struct smart_stargate_board {
    GLOBAL_CONTROL_BLOCK volatile far *gcb;
    unsigned int latch;
    int first_port;
    int number_of_ports;
    StarGate_Board_Type board_type;
} STARGATE_BOARD;

#endif
