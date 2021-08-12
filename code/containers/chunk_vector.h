
template <typename T>
class chunk_vector
{
  struct chunk
  {
    list_entry* Next;
    u32 count;
    T* Vector;
  };

  s32 m_chunkCount;
  s32 m_chunkSize;
  s32 m_count;

  memory_arena* m_arena;

  chunk* Head;

  public:
    chunk_vector(u32 ChunkSize)
    {
    }
    
    u32 Size(){return m_count;};
    b32 IsEmpty(){return m_count == 0;};

    void Clear();
    T* Get(u32 Index)
    {
      if()
    }
    void Insert(u32 Index, T const & Item);
};
