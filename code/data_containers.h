#ifndef DATA_CONTAINER_H
#define DATA_CONTAINER_H

template <class T>
class list
{
	struct entry
	{
		T Data;
		entry* Previous;
		entry* Next;
	};

	entry* Sentinel;
	entry* Position;
	
	memory_arena* Arena;

	u32 Size;

	void InsertBetween( entry* A, entry* B, entry* NewEntry )
	{
		NewEntry->Previous = A;
		NewEntry->Next = B;

		if(A)
		{
			A->Next = NewEntry;
		}

		if(B)
		{
			B->Previous =NewEntry;
		}

		Position = NewEntry;

		++Size;
	}

	entry* AllocateNewEntry(const T& Data)
	{
		entry* Result = (entry*) PushStruct(Arena, entry);
		Result->Data = Data;
		return Result;
	}

	public:
		list() : Sentinel(0), Position(0), Arena(0), Size(0){};

		list(memory_arena* aArena) : Sentinel(0), Position(0), Arena(0), Size(0)
		{
			Initiate(aArena);
		}

		void Initiate( memory_arena* aArena )
		{
			Arena = aArena;
			Sentinel = (entry*) PushStruct(Arena, entry);
			Sentinel->Previous = Sentinel;
			Sentinel->Next = Sentinel;
			Position = Sentinel;
		}

		u32 GetSize(){ return Size; };
		b32 IsEmpty(){ return ( Size==0 ); }
		b32 IsEnd()   { return (Position == Sentinel); };
		

		void First()   { Position = Sentinel->Next;     };
		void Last()    { Position = Sentinel->Previous; };
		void Next()    { if(!IsEnd()) { Position = Position->Next;     } };
		void Previous(){ if(!IsEnd()) { Position = Position->Previous; } };

		void InsertBefore(const T& Data)
		{
			entry* NewEntry = AllocateNewEntry( Data );
			InsertBetween( Position->Previous, Position, NewEntry );
		};

		void InsertAfter(const T& Data)
		{
			entry* NewEntry = AllocateNewEntry( Data );
			InsertBetween( Position, Position->Next, NewEntry );
		}

		void Remove()
		{  
			if(IsEnd()){ return; }

			entry* Base = Position->Previous;
			entry* Next = Position->Next;

			Base->Next = Next;
			Next->Previous = Base;
			
			Position = Base;
			if(Position == Sentinel)
			{
				Position = Sentinel->Next;	
			}
			
			--Size;
		};

		T Get()
		{ 
			return Position->Data; 
		};
};


template <class T>
class fifo_queue
{
	list<T> List;

	public: 

		fifo_queue(){};
		fifo_queue( memory_arena* aArena ) : List(aArena){};
		void Initiate( memory_arena* aArena )
		{
			List.Initiate();
		};

		void Push( const T& Entry )
		{
			List.Last();
			List.InsertAfter(Entry);
		};

		T Pop( )
		{
			List.First();
			T Result = List.Get();
			List.Remove();
			return Result;
		};

		b32 IsEmpty(){ return List.IsEmpty(); };
		u32 GetSize(){ return List.GetSize(); };
};


template <class T>
class filo_queue
{
	list<T> List;

	public: 

		filo_queue( ) : List(){};
		filo_queue( memory_arena* aArena ) : List(aArena){};
		void Initiate( memory_arena* aArena )
		{
			List.Initiate();
		};

		void Push( T& Entry )
		{
			List.Last();
			List.InsertBefore(Entry);
		};

		T Pop( )
		{
			List.First();
			T Result = List.Get();
			List.Remove();
			return Result;
		};

		u32 GetSize(){ return List.GetSize(); };
		b32 IsEmpty(){ return List.IsEmpty(); };
};


#endif