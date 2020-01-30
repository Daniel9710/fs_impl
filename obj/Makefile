CC		= gcc
LD		= gcc

INCDIR	= -I../util/queue \
	  -I../util/monitor \
	  -I../util/dlist \
	  -I../fs \
	  -I../include \

FUSEDIR			= ../fuse
FSDIR			= ../fs
QUEUEDIR		= ../util/queue
DLISTDIR		= ../util/dlist
MONITORDIR		= ../util/monitor

VPATH	= $(FUSEDIR) $(FSDIR) $(QUEUEDIR) $(DLISTDIR) $(MONITORDIR) 

CFLAGS	= $(INCDIR) \
		  -Wall \
		  -D_FILE_OFFSET_BITS=64 \
		  -O2 \
		  -g \
		  -std=c++11 \
		  -MMD \
		  -MP \
		  -Wall \
		  -D_FILE_OFFSET_BITS=64 \
		  -fPIC \
		  -march=native \
#		  -DMONITOR \
#		  -fsanitize=address \

CXX_FLAGS = \
			-O2 \
			-g \
			-std=c++11 \
			-MMD \
			-MP \
			-Wall \
			-DLINUX \
			-D_FILE_OFFSET_BITS=64 \
			-fPIC \
			-march=native \
			

LIBS	= `pkg-config fuse3 --cflags --libs` \
		  -lpthread \


TARGET	= fs_impl

SRCS	= $(foreach dir, .. $(FUSEDIR) $(FSDIR) $(QUEUEDIR) $(DLISTDIR) $(MONITORDIR), $(wildcard $(dir)/*.c))
SRCS	:= $(notdir $(SRCS))

OBJS	= $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $^ -o $(TARGET) $(CFLAGS) $(LIBS) 
	mv $(TARGET) ../

%o: %c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)
	rm -f ../$(TARGET)

depend: $(SRCS)
	$(CC) -M $(CFLAGS) $^ > $@

-include depend
