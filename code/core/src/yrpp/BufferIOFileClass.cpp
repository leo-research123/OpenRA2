#include "filesystem/file_system.hpp"
// Copyright 2020 Electronic Arts Inc. See third_party/ea/LICENSE.TXT.
// Adapted from REDALERT/BFIOFILE.CPP at f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Changes: YRpp names/types; target byte fields; core allocator; host input guards.
#include "yrpp/CCFileClass.h"
#include "yrpp/Memory.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
using std::min;
using std::max;

BufferIOFileClass::BufferIOFileClass() { }

BufferIOFileClass::~BufferIOFileClass(void)
{
	Free();
}

bool BufferIOFileClass::Cache( int size, void * ptr )
RA2_FILE_TRY {
#ifndef RA2_FILES_GAME
    if (size < 0 || (ptr && size < 1024)) return false;
#endif
	if (IOBuffer) {

		if (size || ptr) {
			return( false );
		} else {
			return( true );
		}
	}

	if ( this->Exists(false) ) {
		CachedFileSize = this->GetFileSize();
	} else {
		CachedFileSize = 0;
	}

	if (size) {

		if (size < 1024) {
			size = 1024;

			if (ptr) {
				this->CDCheck(EINVAL, false, nullptr);
			}
		}

		BufferSize = size;
	} else {
		BufferSize = CachedFileSize;
	}

	if ( (size == 0 && ptr) || !BufferSize) {
		return( false );
	}

	if (ptr) {
		IOBuffer = ptr;
	} else {
		IOBuffer = YRMemory::Allocate(BufferSize);
	}

	if (IOBuffer) {
		IsAllocated			= true;
		IsDiskOpen			= false;
		BufferPos			= 0;
		BufferFilePos		= 0;
		BufferChangeBeg	= -1;
		BufferChangeEnd	= -1;
		FilePos				= 0;
		TrueFileStart		= 0;

		if (CachedFileSize) {
			int readsize;
			int opened = false;
			int prevpos = 0;

			if (CachedFileSize <= BufferSize) {
				readsize = CachedFileSize;
			} else {
				readsize = BufferSize;
			}

			if ( this->HasHandle() ) {

				prevpos = this->Seek(0, FileSeekMode::Current);

				if ( RawFileClass::HasHandle() ) RA2_FILE_TRY {
					TrueFileStart = RawFileClass::Seek(0, FileSeekMode::Current);
				} RA2_FILE_FAILURE(0) else {
					TrueFileStart = prevpos;
				}

				if (CachedFileSize <= BufferSize) {

					if (prevpos) {
						this->Seek(0, FileSeekMode::Set);
					}

					BufferPos = prevpos;
				} else {
					BufferFilePos = prevpos;
				}

				FilePos = prevpos;
			} else {
				if ( this->Open(FileAccessMode::Read) ) {
					TrueFileStart = RawFileClass::Seek(0, FileSeekMode::Current);
					opened = true;
				}
			}

			int actual = this->ReadBytes(IOBuffer, readsize);

			if (actual != readsize) {
				this->CDCheck(EIO, false, nullptr);
			}

			if (opened) {
				this->Close();
			} else {

				this->Seek(prevpos, FileSeekMode::Set);
			}

			IsCached = true;
		}

		UseBuffer = true;
		return(true);
	}

	this->CDCheck(ENOMEM, false, nullptr);

	return(false);
} RA2_FILE_FAILURE(0)

void BufferIOFileClass::Free(void)
RA2_FILE_TRY {
	if (IOBuffer) {
		if (IsAllocated) {
			YRMemory::Deallocate(IOBuffer);
			IsAllocated = false;
		}

		IOBuffer = 0;
	}

	BufferSize		= 0;
	IsOpen			= false;
	IsCached			= false;
	IsChanged		= false;
	UseBuffer		= false;
} RA2_FILE_FAILURE()

bool BufferIOFileClass::Commit( void )
RA2_FILE_TRY {
	int size;

	if (UseBuffer) {
		if (IsChanged) {
			size = BufferChangeEnd - BufferChangeBeg;

			if (IsDiskOpen) {
				RawFileClass::Seek( TrueFileStart + BufferFilePos +
										  BufferChangeBeg, FileSeekMode::Set );
				RawFileClass::WriteBytes( IOBuffer, size );
				RawFileClass::Seek( TrueFileStart + FilePos, FileSeekMode::Set );
			} else {
				RawFileClass::Open();
				RawFileClass::Seek( TrueFileStart + BufferFilePos +
										  BufferChangeBeg, FileSeekMode::Set );
				RawFileClass::WriteBytes( IOBuffer, size );
				RawFileClass::Close();
			}

			IsChanged = false;
			return( true );
		} else {
			return( false );
		}
	} else {
		return( false );
	}
} RA2_FILE_FAILURE(0)

char const * BufferIOFileClass::SetFileName(char const * filename)
RA2_FILE_TRY {
	if ( this->GetFileName() && UseBuffer) {
		if ( strcmp(filename, this->GetFileName() ) == 0) {
			return( this->GetFileName() );
		} else {
			Commit();
			IsCached = false;
		}
	}

	RawFileClass::SetFileName(filename);
	return( this->GetFileName() );
} RA2_FILE_FAILURE(nullptr)

bool BufferIOFileClass::Exists(bool shared)
RA2_FILE_TRY {
	if (UseBuffer) {
		return(true);
	}

	return( RawFileClass::Exists(shared) );
} RA2_FILE_FAILURE(0)

bool BufferIOFileClass::HasHandle()
RA2_FILE_TRY {
	if (IsOpen && UseBuffer) {
		return( true );
	}

	return( RawFileClass::HasHandle() );
} RA2_FILE_FAILURE(0)

bool BufferIOFileClass::OpenEx(char const * filename, FileAccessMode rights)
RA2_FILE_TRY {
	this->SetFileName(filename);
	return( BufferIOFileClass::Open( rights ) );
} RA2_FILE_FAILURE(0)

bool BufferIOFileClass::Open(FileAccessMode rights)
RA2_FILE_TRY {
	BufferIOFileClass::Close();

	if (UseBuffer) {

		BufferRights = int(rights);

		if (rights != FileAccessMode::Read ||
			 (rights == FileAccessMode::Read && CachedFileSize > BufferSize) ) {

			if (rights == FileAccessMode::Write) {
				RawFileClass::Open( rights );
				RawFileClass::Close();
				rights = FileAccessMode::Read | FileAccessMode::Write;
				TrueFileStart = 0;
			}

			if (TrueFileStart) {
				UseBuffer = false;
				this->Open(rights);
				UseBuffer = true;
			} else {
				RawFileClass::Open( rights );
			}

			IsDiskOpen = true;

			if (BufferRights == int(FileAccessMode::Write)) {
				CachedFileSize = 0;
			}

		} else {
			IsDiskOpen = false;
		}

		BufferPos			= 0;
		BufferFilePos		= 0;
		BufferChangeBeg	= -1;
		BufferChangeEnd	= -1;
		FilePos				= 0;
		IsOpen				= true;
	} else {
		return RawFileClass::Open( rights ); // YR 431F70 returns the actual result.
	}

	return( true );
} RA2_FILE_FAILURE(0)

int BufferIOFileClass::WriteBytes(void* buffer, int size)
RA2_FILE_TRY {
	int opened = false;

	if ( !this->HasHandle() ) {
		if (!this->Open(FileAccessMode::Write)) {
			return(0);
		}
		TrueFileStart = RawFileClass::Seek(0, FileSeekMode::Current);
		opened = true;
	}

	if (UseBuffer) {
		int sizewritten = 0;

		if (BufferRights != int(FileAccessMode::Read)) {
			while (size) {
				int sizetowrite;

				if (size >= (BufferSize - BufferPos) ) {
					sizetowrite = (BufferSize - BufferPos);
				} else {
					sizetowrite = size;
				}

				if (sizetowrite != BufferSize) {

					if ( !IsCached ) {
						int readsize;

						if (CachedFileSize < BufferSize) {
							readsize = CachedFileSize;
							BufferFilePos = 0;
						} else {
							readsize = BufferSize;
							BufferFilePos = FilePos;
						}

						if (TrueFileStart) {
							UseBuffer = false;
							this->Seek(FilePos, FileSeekMode::Set);
							this->ReadBytes(IOBuffer, BufferSize);
							this->Seek(FilePos, FileSeekMode::Set);
							UseBuffer = true;
						} else {
							RawFileClass::Seek( BufferFilePos, FileSeekMode::Set );
							RawFileClass::ReadBytes( IOBuffer, readsize );
						}

						BufferPos			= 0;
						BufferChangeBeg	= -1;
						BufferChangeEnd	= -1;

						IsCached = true;
					}
				}

				memmove((char *)IOBuffer + BufferPos, (char *)buffer + sizewritten, sizetowrite);

				IsChanged = true;
				sizewritten += sizetowrite;
				size -= sizetowrite;

				if (BufferChangeBeg == -1) {
					BufferChangeBeg = BufferPos;
					BufferChangeEnd = BufferPos;
				} else {
					if (BufferChangeBeg > BufferPos) {
						BufferChangeBeg = BufferPos;
					}
				}

				BufferPos += sizetowrite;

				if (BufferChangeEnd < BufferPos) {
					BufferChangeEnd = BufferPos;
				}

				FilePos = BufferFilePos + BufferPos;

				if (CachedFileSize < FilePos) {
					CachedFileSize = FilePos;
				}

				if (BufferPos == BufferSize) {
					Commit();

					BufferPos = 0;
					BufferFilePos = FilePos;
					BufferChangeBeg = -1;
					BufferChangeEnd = -1;

					if (size && CachedFileSize > FilePos) {
						if (TrueFileStart) {
							UseBuffer = false;
							this->Seek(FilePos, FileSeekMode::Set);
							this->ReadBytes(IOBuffer, BufferSize);
							this->Seek(FilePos, FileSeekMode::Set);
							UseBuffer = true;
						} else {
							RawFileClass::Seek( FilePos, FileSeekMode::Set );
							RawFileClass::ReadBytes( IOBuffer, BufferSize );
						}
					} else {
						IsCached = false;
					}
				}
			}
		} else {
			this->CDCheck(EACCES, false, nullptr);
		}

		size = sizewritten;
	} else {
		size = RawFileClass::WriteBytes(buffer, size);
	}

	if (opened) {
		this->Close();
	}

	return( size );
} RA2_FILE_FAILURE(0)

int BufferIOFileClass::ReadBytes(void * buffer, int size)
RA2_FILE_TRY {
	int opened = false;

	if ( !this->HasHandle() ) {
		if ( this->Open(FileAccessMode::Read) ) {
			TrueFileStart = RawFileClass::Seek(0, FileSeekMode::Current);
			opened = true;
		}
	}

	if (UseBuffer) {
		int sizeread = 0;

		if (BufferRights != int(FileAccessMode::Write)) {
			while (size) {
				int sizetoread;

				if (size >= (BufferSize - BufferPos) ) {
					sizetoread = (BufferSize - BufferPos);
				} else {
					sizetoread = size;
				}

				if ( !IsCached ) {
					int readsize;

					if (CachedFileSize < BufferSize) {
						readsize = CachedFileSize;
						BufferFilePos = 0;
					} else {
						readsize = BufferSize;
						BufferFilePos = FilePos;
					}

					if (TrueFileStart) {
						UseBuffer = false;
						this->Seek(FilePos, FileSeekMode::Set);
						this->ReadBytes(IOBuffer, BufferSize);
						this->Seek(FilePos, FileSeekMode::Set);
						UseBuffer = true;
					} else {
						RawFileClass::Seek( BufferFilePos, FileSeekMode::Set );
						RawFileClass::ReadBytes( IOBuffer, readsize );
					}

					BufferPos			= 0;
					BufferChangeBeg	= -1;
					BufferChangeEnd	= -1;

					IsCached = true;
				}

				memmove((char *)buffer + sizeread, (char *)IOBuffer + BufferPos, sizetoread);

				sizeread += sizetoread;
				size -= sizetoread;
				BufferPos += sizetoread;
				FilePos = BufferFilePos + BufferPos;

				if (BufferPos == BufferSize) {
					Commit();

					BufferPos = 0;
					BufferFilePos = FilePos;
					BufferChangeBeg = -1;
					BufferChangeEnd = -1;

					if (size && CachedFileSize > FilePos) {
						if (TrueFileStart) {
							UseBuffer = false;
							this->Seek(FilePos, FileSeekMode::Set);
							this->ReadBytes(IOBuffer, BufferSize);
							this->Seek(FilePos, FileSeekMode::Set);
							UseBuffer = true;
						} else {
							RawFileClass::Seek( FilePos, FileSeekMode::Set );
							RawFileClass::ReadBytes( IOBuffer, BufferSize );
						}
					} else {
						IsCached = false;
					}
				}
			}
		} else {
			this->CDCheck(EACCES, false, nullptr);
		}

		size = sizeread;
	} else {
		size = RawFileClass::ReadBytes(buffer, size);
	}

	if (opened) {
		this->Close();
	}

	return( size );
} RA2_FILE_FAILURE(0)

int BufferIOFileClass::Seek(int pos, FileSeekMode dir)
RA2_FILE_TRY {
	if (UseBuffer) {
		bool adjusted = false;

		switch (dir) {
			case FileSeekMode::End:
				FilePos = CachedFileSize;
				break;

			case FileSeekMode::Set:
				FilePos = 0;
				break;

			case FileSeekMode::Current:
			default:
				break;
		}

		if (TrueFileStart) {
			if (pos >= TrueFileStart) {
				pos -= TrueFileStart;
				adjusted = true;
			}
		}

		FilePos += pos;

		if (FilePos < 0) {
			FilePos = 0;
		}

		if (FilePos > CachedFileSize ) {
			FilePos = CachedFileSize;
		}

		if (CachedFileSize <= BufferSize) {
			BufferPos = FilePos;
		} else {
			if (FilePos >= BufferFilePos &&
				 FilePos < (BufferFilePos + BufferSize) ) {
				BufferPos = FilePos - BufferFilePos;
			} else {
				Commit();

				if (TrueFileStart) {
					UseBuffer = false;
					this->Seek(FilePos, FileSeekMode::Set);
					UseBuffer = true;
				} else {
					RawFileClass::Seek(FilePos, FileSeekMode::Set);
				}

				IsCached = false;
			}
		}

		if (TrueFileStart && adjusted) {
			return( FilePos + TrueFileStart );
		}

		return( FilePos );
	}

	return( RawFileClass::Seek(pos, dir) );
} RA2_FILE_FAILURE(0)

int BufferIOFileClass::GetFileSize(void)
RA2_FILE_TRY {
	if (IsOpen && UseBuffer) {
		return( CachedFileSize );
	}

	return( RawFileClass::GetFileSize() );
} RA2_FILE_FAILURE(0)

void BufferIOFileClass::Close(void)
RA2_FILE_TRY {
	if (UseBuffer) {
		Commit();

		if (IsDiskOpen) {

			if (TrueFileStart) {
				UseBuffer = false;
				this->Close();
				UseBuffer = true;
			} else {
				RawFileClass::Close();
			}

			IsDiskOpen = false;
		}

		IsOpen = false;
	} else {
		RawFileClass::Close();
	}
} RA2_FILE_FAILURE()
