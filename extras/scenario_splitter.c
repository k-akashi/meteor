#include "deltaQ.h"

#include <getopt.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ERROR   -1
#define SUCCESS  0
#define PARAMS_TOTAL        19
#define PROG_NAME           "scenario_splitter"

int32_t time_recs;
char *ofile_name_base;

void
usage()
{
    fprintf(stderr, "scnerio_splitter -i input_file -o output_file_basename\n");
}

int
scenario_split(FILE *ifile_fd)
{
    char *ofilename;
    int32_t bin_rec_max_cnt;
    int fd_i, time_i, rec_i;
    void **ofile_fds;
    FILE *tmp_fd;
    bin_hdr_cls bin_hdr;
    bin_time_rec_cls bin_time_rec;
    bin_rec_cls *recs = NULL;

    ofilename = malloc(MAX_STRING);
    memset(ofilename, '\0', MAX_STRING);
    if(io_binary_read_header_from_file(&bin_hdr, ifile_fd) == ERROR) {
        fprintf(stderr, "Aborting on input error (binary header)");
        fclose(ifile_fd);
        exit(1);
    }

    printf("* HEADER INFORMATION:\n");
    io_binary_print_header(&bin_hdr);

    bin_rec_max_cnt = bin_hdr.interface_number * (bin_hdr.interface_number - 1);
    recs = (bin_rec_cls *)calloc(bin_rec_max_cnt, sizeof(bin_rec_cls));
    if(recs == NULL) {
        fprintf(stderr, "Cannot allocate memory for records");
        fclose(ifile_fd);
        exit(1);
    }

    ofile_fds = malloc((bin_hdr.interface_number + 1) * sizeof (FILE *));
    for (fd_i = 0; fd_i <= bin_hdr.interface_number; fd_i++) {
        sprintf(ofilename, "%s_%04d.xml.bin", ofile_name_base, fd_i);
        tmp_fd = fopen(ofilename, "w");
        ofile_fds[fd_i] = tmp_fd;
    }

    for (fd_i = 0; fd_i < bin_hdr.interface_number; fd_i++) {
        io_binary_write_header_to_file(bin_hdr.interface_number, bin_hdr.time_record_number,
                bin_hdr.major_version, bin_hdr.minor_version,
                bin_hdr.subminor_version, bin_hdr.svn_revision, 
                ofile_fds[fd_i]);
    }

    int record_number;
    int *connection_count = malloc(sizeof (int) * bin_hdr.interface_number);
    memset(connection_count, 0, bin_hdr.interface_number);

    for(time_i = 0; time_i < bin_hdr.time_record_number; time_i++) {
        if (io_binary_read_time_record_from_file(&bin_time_rec, ifile_fd) == ERROR) {
            fprintf(stderr, "Aborting on input error (time record)");
            fclose(ifile_fd);
            exit(1);
        }
        if (io_binary_read_records_from_file(recs, bin_time_rec.record_number, ifile_fd) == ERROR) {
            printf("Aborting on input error (records)\n");
            fclose(ifile_fd);
            exit(1);
        }

        record_number = bin_time_rec.record_number;
        for (rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
            connection_count[recs[rec_i].to_id]++;
        }
        /*
        for (fd_i = 0; fd_i < bin_hdr.interface_number; fd_i++) {
            printf("connection %d: %d\n", fd_i, connection_count[fd_i]);
        }
        io_binary_print_time_record(&bin_time_rec);
        */

        for (fd_i = 0; fd_i < bin_hdr.interface_number; fd_i++) {
            bin_time_rec.record_number = connection_count[fd_i];
            io_binary_write_time_record_to_file2(&bin_time_rec, ofile_fds[fd_i]);
        }
        for (rec_i = 0; rec_i < record_number; rec_i++) {
            //io_binary_print_record(&recs[rec_i]);
            io_binary_write_record_to_file2(&recs[rec_i], ofile_fds[recs[rec_i].to_id]);
        }

        memset(connection_count, 0, sizeof (int) * bin_hdr.interface_number);
    }

    for (fd_i = 0; fd_i < bin_hdr.interface_number; fd_i++) {
        fclose(ofile_fds[fd_i]);
    }

    return 0;
}

int
main(int argc, char **argv)
{
    FILE *ifile_fd;
    char c;

    if(argc < 2) {
        usage();
        exit(1);
    }

    while((c = getopt(argc, argv, "hi:o:")) != -1) {
        switch(c) {
        case 'h':
            usage();
            exit(0);
        case 'i':
            ifile_fd = fopen(optarg, "r");
            break;
        case 'o':
            ofile_name_base = optarg;
            break;
        default: /* '?' */
            usage();
            exit(1);
        }
    }

    if(ifile_fd == NULL) {
        usage();
        exit(1);
    }
    scenario_split(ifile_fd);

    fclose(ifile_fd);

    return 0;
}
