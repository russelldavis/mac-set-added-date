// Adapted from https://apple.stackexchange.com/a/457371/381049

#include <stdlib.h>
#include <string.h>
#include <sys/attr.h>
#include <unistd.h>
#include <stdio.h>

/*
 * For a list of files set "Date Added" in
 * macOS Finder to match "Date Modified"
 */

/*
 * Get kMDItemDateAdded of path.
 *
 * Returns:
 *   • 0 on success
 *   • 1 if a system call failed: check errno
 *   • 2 if something else went wrong
 */
int get_date_modified(const char* path, struct timespec * out) {

    attrgroup_t request_attrs = ATTR_CMN_RETURNED_ATTRS | ATTR_CMN_MODTIME;

    struct attrlist request;
    memset(&request, 0, sizeof(request));
    request.bitmapcount = ATTR_BIT_MAP_COUNT;
    request.commonattr = request_attrs;

    typedef struct {
        u_int32_t length;
        attribute_set_t returned;
        struct timespec modified;
    } __attribute__((aligned(4), packed)) response_buf_t;

    response_buf_t response;

    int err = getattrlist(path, &request, &response, sizeof(response), 0);
    if (err != 0) {
        return 1;
    }
    if (response.length != sizeof(response)) {
        // Need a different-sized buffer; but provided one of exactly required
        // size?!
        return 2;
    }
    if (response.returned.commonattr != request_attrs) {
        // Didn’t get back all requested common attributes
        return 2;
    }

    out->tv_sec = response.modified.tv_sec;
    out->tv_nsec = response.modified.tv_nsec;

    return 0;
}

/*
 * Set kMDItemDateAdded of path.
 *
 * Returns:
 *   • 0 on success
 *   • 1 if a system call failed: check errno
 */
int set_date_added(const char* path, struct timespec in) {
    attrgroup_t request_attrs = ATTR_CMN_ADDEDTIME;

    struct attrlist request;
    memset(&request, 0, sizeof(request));
    request.bitmapcount = ATTR_BIT_MAP_COUNT;
    request.commonattr = request_attrs;

    typedef struct {
        struct timespec added;
    } __attribute__((aligned(4), packed)) request_buf_t;

    request_buf_t request_buf;
    request_buf.added.tv_sec = in.tv_sec;
    request_buf.added.tv_nsec = in.tv_nsec;

    int err = setattrlist(path, &request, &request_buf, sizeof(request_buf), 0);
    if (err != 0) {
        return 1;
    }

    return 0;
}

/*
 * Set kMDItemDateAdded of path to kMDItemFSContentChangeDate
 *
 * Returns:
 *   • 0 on success
 *   • 1 if a path doesn't exist
 *   • 2 if a function call failed
 */
int set_added_date_from_modified(char* path) {

    int err;

    if(access(path, F_OK) != 0) {
        printf("error: %s doesn't exist\n", path);
        return 1;
    }

    struct timespec out;
    err = get_date_modified(path, &out);
    if (err != 0) {
        return 2;
    }

    struct timespec in;
    in.tv_sec = out.tv_sec;
    in.tv_nsec = out.tv_nsec;
    err = set_date_added(path, in);
    if (err != 0) {
        return 2;
    }

    return 0;
}


/*
 * Modify paths given as command line arguments
 *
 * Returns:
 *   • 0 on success
 *   • 1 if a path doesn't exist
 *   • 2 if something else went wrong
 */
int main(int argc, char **argv) {

    if (argc >= 2) {
        for (int i = 1; i < argc; ++i) {
            char *path = argv[i];
            int err = set_added_date_from_modified(path);
            if (err == 0) {
                printf("%s\n", path);
            }
            else if (err == 2) {
                printf("an unknown error occured\n");
                return 2;
            }
        }
    }
    else {
        printf("usage: set-added-date-from-modified file1 file2 ...\n");
    }

    return EXIT_SUCCESS;
}
