enum pdbStreamVersions{
    VC2 = 19941610,
    VC4 = 19950623,
    VC41 = 19950814,
    VC50 = 19960307,
    VC98 = 19970604,
    VC70Dep = 19990604,
    VC70 = 20000404,
    VC80 = 20030901,
    VC110 = 20091201,
    VC140 = 20140508,
};
 
struct pdbStreamHeader{
    u32 version;
    u32 signature;
    u32 age;
    u64 guid_high;
    u64 guid_low;
};

struct pdbTPIHeader{
    u32 version;
    u32 header_size;

    u32 type_index_begin;
    u32 type_index_end;
    u32 type_record_bytes;

    u16 hash_stream_index;
    u16 hash_aux_stream_index;
    u32 hash_key_size;
    u32 num_hash_buckets;

    s32 hash_value_buffer_offset;
    u32 hash_value_buffer_length;

    s32 index_offset_buffer_offset;
    u32 index_offset_buffer_length;

    s32 hash_adj_buffer_offset;
    u32 hash_adj_buffer_length;
};

struct pdbDBIHeader{
    s32 version_signature;
    u32 version_header;
    u32 age;
    u16 global_stream_index;
    u16 build_number;
    u16 public_stream_index;
    u16 pdb_dll_version;
    u16 sym_record_stream;
    u16 pdb_dll_rbld;
    s32 mod_info_size;
    s32 section_contribution_size;
    s32 section_map_size;
    s32 source_info_size;
    s32 type_server_map_size;
    u32 mfc_type_server_index;
    s32 optional_dbg_header_size;
    s32 ec_substream_size;
    u16 flags;
    u16 machine;
    u32 padding;
};

struct msfSuperblock{
    char magic[32];
    u32 block_size;
    u32 free_block_map_block;
    u32 num_blocks;
    u32 num_directory_bytes;
    u32 unknown;
    u32 block_map_addr;
};

struct msfStreamBlock{
    u32 count;
    u32* blocks;
};

struct msfStreamDirectory{
    u32 num_streams;
    u32* stream_sizes;
    msfStreamBlock* stream_blocks;
};

u8* stitch_blocks(u32* blocks, u32 count, u8* base, u64 blocksize){
    if(!blocks || !count) return 0;
    u8* stitch = (u8*)memalloc(blocksize*count);
    forI(count) memcpy(stitch+i*blocksize, base+blocks[i]*blocksize, blocksize);
    return stitch;
}

void parse_pdb(str8 path){
    File* file = file_init(path, FileAccess_Read);
    u8* read = file_read_alloc(file, file->bytes, deshi_allocator).str;
    //get superblock
    msfSuperblock superblock = *(msfSuperblock*)read;
    u32 blocksize = superblock.block_size;
    //find and build stream directory 
    // sd_ -> streamdirectory_
    msfStreamDirectory sd;
    //stitch together the stream directory
    //im not sure if this is necessary. rn it seems like the SD is contiguous, but a lot of other stuff isnt
    //so we'll do this to be safe
    //this could also be changed to use nodes or something later so we dont have to copy memory
    u32* sd_map_ptr   = (u32*)(read+blocksize*superblock.block_map_addr);
    u32  sd_map_count = ceil(f32(superblock.num_directory_bytes)/blocksize);
    u8*  sd_stitch    = stitch_blocks(sd_map_ptr, sd_map_count, read, blocksize);//(u8*)memalloc(blocksize*sd_map_count);
    //forI(sd_map_count){
    //    memcpy(sd_stitch+i*blocksize, read+(*(sd_map_ptr+i))*blocksize, blocksize);
    //}
    u32* sd_ptr = (u32*)sd_stitch;
    
    sd.num_streams = *sd_ptr;
    sd.stream_sizes = sd_ptr+1;
    sd.stream_blocks = (msfStreamBlock*)memalloc(sizeof(msfStreamBlock)*sd.num_streams);
    u32 blockssum = 0;
    forI(sd.num_streams){
        u32 nblocks = (u32)ceil(f32(sd.stream_sizes[i])/superblock.block_size);
        sd.stream_blocks[i] = { nblocks, sd_ptr+sd.num_streams+blockssum };
        blockssum+=nblocks;
    }
    //check that stream directory is valid
    if((blockssum+sd.num_streams+1)*4 == superblock.num_directory_bytes) LogS("pdbparse", "stream directory byte count and num_directory_bytes match!");
    else LogE("pdbparse", "stream directory byte count and num_directory_bytes do NOT match");

    pdbStreamHeader pdbheader;
    forI(sd.num_streams){
        string out;
        if(!sd.stream_blocks[i].count){
            out = toStr("stream ", i, ": ", sd.stream_sizes[i], " | ", "no blocks");
        }
        else {
            out = toStr("stream ", i, ": ", sd.stream_sizes[i], " | ", "{ ");
            forX(j,sd.stream_blocks[i].count){
                if(j!=sd.stream_blocks[i].count-1)
                    out += toStr(sd.stream_blocks[i].blocks[j], ", ");
                else
                    out += toStr(sd.stream_blocks[i].blocks[j], " }"); 
            }
        }
        Log("pdbparse", out);
    }

    pdbDBIHeader tpi;
    forI(sd.num_streams){
        u8* stream = stitch_blocks(sd.stream_blocks[i].blocks, sd.stream_blocks[i].count, read, blocksize);
        if(stream)
            tpi = *(pdbDBIHeader*)stream;

    }


    return;
}


