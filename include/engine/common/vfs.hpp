#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <cstdio>
#include <climits>

#include "engine/common/tcf.h"

enum LoadMode {
    OnDemand,
    All,
};

struct LoadedFile {
    std::vector<uint8_t> data;
    uint64_t size() const;
    const uint8_t* data_ptr() const;
};

struct MountPoint {
    std::string mount_path;
    std::string source_path;
    bool is_archive;
    LoadMode load_mode;
    int priority;
    uint64_t mount_order;
    tcf_vfs_t* tcf_handle;

    std::unordered_map<std::string, std::unique_ptr<LoadedFile>> loaded_files;

    MountPoint(const std::string& mount, const std::string& source, 
               bool archive, LoadMode mode, int priority, uint64_t mount_order);
    
    ~MountPoint();
};

struct VirtualFile {
    MountPoint* mount;
    std::string path;
    uint64_t position;
    uint64_t size;

    tcf_file_t* tcf_handle;    
    FILE* dir_handle;         

    const LoadedFile* loaded_data;

    bool eof;
    bool error;
    
    VirtualFile();
    ~VirtualFile();
};

namespace CE::VFS::Returns {
    extern const int LOAD_SUCCESS;
    extern const int LOAD_FAIL;
    extern const int NO_SUCH_FILE_OR_DIRECTORY;
}

namespace CE::VFS {
    class VFS {        
    public:
        // Mounts are resolved by:
        //  1) most-specific virtual mount path
        //  2) higher priority (larger value wins)
        //  3) newer mount (later call wins)
        int MountArchive(const char* archive_path, const char* v_mount_path, const LoadMode loadmode, int priority = 0);

        int MountFolder(const char* folder_path, const char* v_mount_path, const LoadMode loadmode, int priority = 0);

        bool Unmount(const char* v_mount_path);

        bool FileExists(const char* virtual_path);

        bool GetFileSize(const char* virtual_path, uint64_t& out_size);

        VirtualFile* OpenFile(const char* virtual_path);

        size_t ReadFile(VirtualFile* file, void* buffer, size_t size);

        bool SeekFile(VirtualFile* file, int64_t offset, int whence);

        int64_t TellFile(VirtualFile* file);

        void CloseFile(VirtualFile* file);

        void ListMounts();

        VirtualFile* V_fopen(const char* virtual_path, const char* mode);
        size_t V_fread(void* ptr, size_t size, size_t nmemb, VirtualFile* stream);
        int V_fseek(VirtualFile* stream, long offset, int whence);
        long V_ftell(VirtualFile* stream);
        int V_fclose(VirtualFile* stream);
        int V_feof(VirtualFile* stream);
        int V_ferror(VirtualFile* stream);
        void V_clearerr(VirtualFile* stream);
        void V_rewind(VirtualFile* stream);
        int V_fgetc(VirtualFile* stream);
        char* V_fgets(char* s, int n, VirtualFile* stream);

    private:
        std::vector<std::unique_ptr<MountPoint>> mounts;
        uint64_t next_mount_order = 1;

        std::string NormalizePath(const std::string& path);

        std::string GetRelativePath(const std::string& virtual_path, 
                                    const std::string& mount_path);

        MountPoint* FindMount(const std::string& virtual_path, std::string& relative_path);

        bool LoadEntireFolder(MountPoint* mount);

        bool LoadEntireArchive(MountPoint* mount);

        bool LoadFromTCF(MountPoint* mount, const std::string& rel_path, 
                         std::vector<uint8_t>& out_data);

        bool load_file_from_directory(MountPoint* mount, const std::string& rel_path,
                                     std::vector<uint8_t>& out_data);
    };

    inline VirtualFile* virtual_fopen(VFS* vfs, const char* virtual_path, const char* mode)
    {
        return vfs ? vfs->V_fopen(virtual_path, mode) : nullptr;
    }
    inline size_t virtual_fread(VFS* vfs, void* ptr, size_t size, size_t nmemb, VirtualFile* stream)
    {
        return vfs ? vfs->V_fread(ptr, size, nmemb, stream) : 0;
    }
    inline int virtual_fseek(VFS* vfs, VirtualFile* stream, long offset, int whence)
    {
        return vfs ? vfs->V_fseek(stream, offset, whence) : -1;
    }
    inline long virtual_ftell(VFS* vfs, VirtualFile* stream)
    {
        return vfs ? vfs->V_ftell(stream) : -1;
    }
    inline int virtual_fclose(VFS* vfs, VirtualFile* stream)
    {
        return vfs ? vfs->V_fclose(stream) : -1;
    }
}
