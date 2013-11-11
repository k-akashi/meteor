
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 * 
 * See the file 'LICENSE' for licensing information.
 *
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: show_bin.c
 * Function: Display binary QOMET output in human-readable form
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *   Chages: Kunio AKASHI
 *
 ***********************************************************************/


#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include "deltaQ.h"


///////////////////////////////////
// Generic variables and functions
///////////////////////////////////

#define MESSAGE_WARNING
//#define MESSAGE_DEBUG
#define MESSAGE_INFO

#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                          \
  fprintf(stderr, "show_bin WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stderr, message); fprintf(stderr,"\n");                         \
} while(0)
#else
#define WARNING(message...)	/* message */
#endif

#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                          \
  fprintf(stdout, "show_bin DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                       \
} while(0)
#else
#define DEBUG(message...)	/* message */
#endif

#ifdef MESSAGE_INFO
#define INFO(message...) do {                                       \
  fprintf(stdout, message); fprintf(stdout,"\n");                   \
} while(0)
#else
#define INFO(message...)	/* message */
#endif


#define PRINT_SC        0
#define PRINT_GNUPLOT   1

// print usage info
static void
usage()
{
    fprintf(stderr, "\nshow_bin. Display binary QOMET output as text.\n\n");
    fprintf(stderr, "Usage: show_bin -b <scenario_file.xml.bin> [-t gnuplot] [-s src_id] [-d dst_id]\n");
    fprintf(stderr, "** gnuplot types output format is follow.\n");
    fprintf(stderr, "     time, from_id, to_id delay, lossrate, bandwidth\n");
}

void
print_systeminfo()
{
    INFO("\nshow_bin. Display binary QOMET output as text.\n");

    printf("------------------------------------------------------------------------\n");
    printf("System information:     ");
#ifdef __linux__
    printf("Linux OS, ");
#elif __freebsd__
    printf("FreeBSD OS, ");
#elif __macosx__
    printf("Mac OS X, ");
#else
    printf("Unknown OS, ");
#endif

#if __WORDSIZE == 32
    printf("32 bit architecture\n");
#elif __WORDSIZE == 64
    printf("64 bit architecture\n");
#else
    printf("unknown architecture\n");
#endif
    printf("Variable sizes [bytes]: ");

    //#if __WORDSIZE == 64
    //#define PRI_SIZEOF "lu"
    //#else
    //#define PRI_SIZEOF "u"
    //#endif

    printf("char=%zu  short=%zu  int=%zu  long=%zu  float=%zu  double=%zu\n",
        sizeof (char), sizeof (short), sizeof (int), sizeof (long),
	    sizeof (float), sizeof (double));

    printf("Data structure [bytes]: ");
    printf("bin_hdr_cls=%zu  bin_time_rec_cls=%zu bin_rec_cls=%zu\n", 
        sizeof(struct bin_hdr_cls), sizeof(struct bin_time_rec_cls), sizeof(struct bin_rec_cls));
    printf("------------------------------------------------------------------------\n");

}

int
main(int argc, char *argv[])
{
    // binary file name and descriptor
    char c;
    char bin_filename[MAX_STRING];
    FILE *bin_file;

    // binary file header data structure
    struct bin_hdr_cls bin_hdr;

    // binary file time record data structure
    struct bin_time_rec_cls binary_time_record;

    // binary file record data structure and max count
    struct bin_rec_cls *bin_recs = NULL;
    uint32_t bin_rec_max_cnt;

    // counters for time and binary records
    uint64_t time_i;
    uint32_t rec_i;
    int32_t type = PRINT_SC;
    int32_t src_id, dst_id;


    src_id = -1;
    dst_id = -1;

    if(argc <= 1) {
        WARNING("No binary QOMET output file was provided");
        usage(stdout);
        exit(1);
    }

    while((c = getopt(argc, argv, "b:d:hs:t:")) != -1) {
        switch(c) {
            case 'b':
                strncpy(bin_filename, optarg, MAX_STRING - 1);
                bin_file = fopen(optarg, "r");
                break;
            case 'd':
                dst_id = atoi(optarg);
                break;
            case 'h':
                usage();
                exit(0);
                break;
            case 's':
                src_id = atoi(optarg);
                break;
            case 't':
                if(strcmp(optarg, "gnuplot") == 0) {
                    type = PRINT_GNUPLOT;
                }
                else {
                    fprintf(stderr, "set type : gnuplot\n");
                    usage();
                    exit(1);
                }
                break;
            default:
                usage();
                exit(1);
        }
    }

    print_systeminfo();

    INFO("\nShowing file '%s'...\n", bin_filename);

    if(bin_file == NULL) {
        WARNING("Cannot open binary file '%s'", bin_filename);
        exit(1);
    }

    if(io_binary_read_header_from_file (&bin_hdr, bin_file) == ERROR) {
        WARNING("Aborting on input error (binary header)");
        fclose(bin_file);
        exit(1);
    }

    printf("* HEADER INFORMATION:\n");
    io_binary_print_header(&bin_hdr);

    bin_rec_max_cnt = bin_hdr.if_num * (bin_hdr.if_num - 1);
    bin_recs = (struct bin_rec_cls *)calloc(bin_rec_max_cnt, sizeof(struct bin_rec_cls));
    if(bin_recs == NULL) {
        WARNING ("Cannot allocate memory for records");
        fclose(bin_file);
        exit(1);
    }

    printf("* RECORD CONTENT:\n");
    for(time_i = 0; time_i < bin_hdr.time_rec_num; time_i++) {
        // read time record
        if(io_binary_read_time_record_from_file(&binary_time_record, bin_file) == ERROR) {
            WARNING("Aborting on input error (time record)");
            fclose(bin_file);
            exit(1);
        }
        io_binary_print_time_record(&binary_time_record);
        if(binary_time_record.record_number > bin_rec_max_cnt) {
            WARNING("Number of records exceeds maximum (%d)", bin_rec_max_cnt);
            fclose(bin_file);
            exit(1);
        }

        if(io_binary_read_records_from_file(bin_recs, binary_time_record.record_number, bin_file) == ERROR) {
            WARNING("Aborting on input error (records)");
            fclose(bin_file);
            exit(1);
        }

        for(rec_i = 0; rec_i < binary_time_record.record_number; rec_i++) {
            if(src_id == -1 || src_id == bin_recs[rec_i].from_id) {
                if(dst_id == -1 || dst_id == bin_recs[rec_i].to_id) {
                    if(type == PRINT_SC) {
                        io_binary_print_record(&bin_recs[rec_i]);
                    }
                    else if(type == PRINT_GNUPLOT) {
                        io_bin_rec2gnuplot(&bin_recs[rec_i], time_i);
                    }
                }
            }
        }
    }

    free(bin_recs);
    fclose(bin_file);

    return 0;
}
