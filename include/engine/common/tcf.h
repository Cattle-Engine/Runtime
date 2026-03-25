#ifndef TCF_H
#define TCF_H

#include <stdint.h>
#include <stddef.h>

#define TCF_OK                0
#define TCF_ERR_IO           -1
#define TCF_ERR_FORMAT       -2
#define TCF_ERR_CRC          -3
#define TCF_ERR_MEMORY       -4
#define TCF_ERR_NOT_FOUND    -5
#define TCF_ERR_ARG          -6

#ifdef __cplusplus
extern "C" {
#endif

int tcf_extract(const char *tcf_path, const char *output_dir);
int tcf_pack(const char *input_dir, const char *out_path);

/* Loads a single file from a .tcf into memory.
 * - inner_path is the archive path (forward slashes).
 * - out_data is heap-allocated with malloc; free with tcf_free().
 */
int tcf_load_file(const char *tcf_path,
                  const char *inner_path,
                  uint8_t **out_data,
                  uint64_t *out_size);

void tcf_free(void *ptr);

typedef struct tcf_vfs_t tcf_vfs_t;
typedef struct tcf_file_t tcf_file_t;

int tcf_vfs_open(const char *tcf_path, tcf_vfs_t **vfs_out);
void tcf_vfs_close(tcf_vfs_t *vfs);

int tcf_vfs_open_file(tcf_vfs_t *vfs, const char *inner_path, tcf_file_t **file_out);
int tcf_vfs_read(tcf_file_t *file, void *buffer, size_t size, size_t *bytes_read);
int tcf_vfs_seek(tcf_file_t *file, int64_t offset, int whence);
int64_t tcf_vfs_tell(tcf_file_t *file);
void tcf_vfs_close_file(tcf_file_t *file);
uint64_t tcf_vfs_file_size(tcf_file_t *file);

/* Enumerate archive entries without allocating.
 * Pointers returned by tcf_vfs_entry_info remain valid until tcf_vfs_close().
 */
uint64_t tcf_vfs_entry_count(const tcf_vfs_t *vfs);
int tcf_vfs_entry_info(const tcf_vfs_t *vfs,
                       uint32_t index,
                       const char **path_out,
                       uint32_t *path_len_out,
                       uint64_t *size_out);

/* Reads and unshifts the entire entry (index) into buffer.
 * buffer_size must be >= entry size.
 */
int tcf_vfs_read_entry(tcf_vfs_t *vfs, uint32_t index, void *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif 
