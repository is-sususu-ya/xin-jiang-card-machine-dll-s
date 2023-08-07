/*
 * =====================================================================================
 *
 *       Filename:  utils_input.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2019-03-25 09:28:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:   (), 
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef  UTILS_INPUT_H
#define  UTILS_INPUT_H

int input_read( int fd, char *buf , int maxsize );
int open_input_dev( const char *input_name );
int input_ready( int fd, int tout );

#endif  // UTILS_INPUT_H
