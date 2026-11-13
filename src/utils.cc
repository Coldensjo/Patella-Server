#include <cstdint>
#include <cstdlib>

#include "utils.hh"

TPrintFunction *PrintFunction = nullptr;	// utils.c:15
TErrorFunction *ErrorFunction = nullptr;	// utils.c:16
char StandardLogfile[4096] = "";	// utils.c:17

// 0x0804cfc0  utils.c:28
void SetPrintFunction(TPrintFunction * /*Function*/) { abort(); /* STUB */ }

// 0x0804cfd0  utils.c:36
void SetErrorFunction(TErrorFunction * /*Function*/) { abort(); /* STUB */ }

// 0x0804cfe0  utils.c:49
void SilentHandler(int, char *) { abort(); /* STUB */ }

// 0x0804cff0  utils.c:56
void SilentHandler(char *) { abort(); /* STUB */ }

// 0x0804d000  utils.c:63
void LogfileHandler(int, char * /*Text*/) { abort(); /* STUB */ }

// 0x0804d080  utils.c:75
void LogfileHandler(char * /*Text*/) { abort(); /* STUB */ }

// 0x0804d100  utils.c:92
void print(int /*Level*/, char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x0804d160  utils.c:107
void error(char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x0804d1c0  utils.c:127
bool TReadStream::readFlag() { abort(); /* STUB */ }

// 0x0804d200  utils.c:135
unsigned short TReadStream::readWord() { abort(); /* STUB */ }

// 0x0804d250  utils.c:145
uint32_t TReadStream::readQuad() { abort(); /* STUB */ }

// 0x0804d2d0  utils.c:159
void TReadStream::readString(char * /*Buffer*/, int /*MaxLength*/) { abort(); /* STUB */ }

// 0x0804d3c0  utils.c:182
void TReadStream::readBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x0804d460  utils.c:199
void TWriteStream::writeFlag(bool /*f*/) { abort(); /* STUB */ }

// 0x0804d4a0  utils.c:207
void TWriteStream::writeWord(unsigned short /*w*/) { abort(); /* STUB */ }

// 0x0804d500  utils.c:216
void TWriteStream::writeQuad(uint32_t /*q*/) { abort(); /* STUB */ }

// 0x0804d580  utils.c:230
void TWriteStream::writeString(char * /*s*/) { abort(); /* STUB */ }

// 0x0804d630  utils.c:248
void TWriteStream::writeBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x0804d730  utils.c:265
TReadBuffer::TReadBuffer(unsigned char * /*data*/, int /*size*/) { abort(); /* STUB */ }

// 0x0804d790  utils.c:285
unsigned char TReadBuffer::readByte() { abort(); /* STUB */ }

// 0x0804d800  utils.c:296
unsigned short TReadBuffer::readWord() { abort(); /* STUB */ }

// 0x0804d880  utils.c:307
uint32_t TReadBuffer::readQuad() { abort(); /* STUB */ }

// 0x0804d910  utils.c:318
void TReadBuffer::readBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x0804d9c0  utils.c:332
bool TReadBuffer::eof() { abort(); /* STUB */ }

// 0x0804d9e0  utils.c:340
void TReadBuffer::skip(int /*Count*/) { abort(); /* STUB */ }

// 0x0804dab0  utils.c:354
TWriteBuffer::TWriteBuffer(unsigned char * /*data*/, int /*size*/) { abort(); /* STUB */ }

// 0x0804db10  utils.c:373
void TWriteBuffer::writeByte(unsigned char /*b*/) { abort(); /* STUB */ }

// 0x0804db80  utils.c:383
void TWriteBuffer::writeWord(unsigned short /*w*/) { abort(); /* STUB */ }

// 0x0804dc00  utils.c:394
void TWriteBuffer::writeQuad(uint32_t /*q*/) { abort(); /* STUB */ }

// 0x0804dca0  utils.c:410
void TWriteBuffer::writeBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x0804dd60  utils.c:422
TDynamicWriteBuffer::TDynamicWriteBuffer(int InitialSize) : TWriteBuffer(new unsigned char[InitialSize], InitialSize) { abort(); /* STUB */ }

// 0x0804ddd0  utils.c:433
TDynamicWriteBuffer::~TDynamicWriteBuffer() { abort(); /* STUB */ }

// 0x0804de40  utils.c:442
void TDynamicWriteBuffer::resizeBuffer() { abort(); /* STUB */ }

// 0x0804dea0  utils.c:456
void TDynamicWriteBuffer::writeByte(unsigned char /*b*/) { abort(); /* STUB */ }

// 0x0804df50  utils.c:466
void TDynamicWriteBuffer::writeWord(unsigned short /*w*/) { abort(); /* STUB */ }

// 0x0804e010  utils.c:477
void TDynamicWriteBuffer::writeQuad(uint32_t /*q*/) { abort(); /* STUB */ }

// 0x0804e0e0  utils.c:493
void TDynamicWriteBuffer::writeBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x0804e1a0  utils.c:509
int val(char /*c*/) { abort(); /* STUB */ }

// 0x0804e1f0  utils.c:520
int random(int /*low*/, int /*high*/) { abort(); /* STUB */ }

// 0x0804e230  utils.c:530
bool FileExists(char * /*FileName*/) { abort(); /* STUB */ }

// 0x0804e290  utils.c:544
void TreatAllFiles(char * /*Directory*/, char * /*Extension*/, TFileHandlerFunction * /*Function*/) { abort(); /* STUB */ }

// 0x0804e4a0  utils.c:566
// local static: char *mailer = "/usr/sbin/sendmail"  (utils.c:567, 0x08126740 .data)
// local static: char *envelope = "auto-responder@tibia.com"  (utils.c:568, 0x08126744 .data)
// local static: char *replyto = "Tibia <cip@tibia.com>"  (utils.c:569, 0x08126748 .data)
// local static: char *sender = "Tibia <auto-responder@tibia.com>"  (utils.c:570, 0x0812674c .data)
// local static: char *xmailer = "Tibia"  (utils.c:571, 0x08126750 .data)
void SendMail(char * /*Address*/, char * /*Subject*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x0804e910  utils.c:637
void RemoveZombie() { abort(); /* STUB */ }

// 0x0804e940  utils.c:646
uint32_t PackAbsoluteCoordinate(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0804e970  utils.c:654
uint32_t PackRelativeCoordinate(int /*dx*/, int /*dy*/, int /*dz*/) { abort(); /* STUB */ }

// 0x0804e9a0  utils.c:662
void UnpackAbsoluteCoordinate(uint32_t /*Code*/, int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x0804e9e0  utils.c:672
void UnpackRelativeCoordinate(uint32_t /*Code*/, int & /*dx*/, int & /*dy*/, int & /*dz*/) { abort(); /* STUB */ }
