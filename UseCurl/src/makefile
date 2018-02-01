
TARGET_BIN = UseCurl.bin

#c源文件目录
C_FILE=$(wildcard\
		./*.c\
		)

#cpp源文件目录		
CPP_FILE = $(wildcard\
		./*.cpp\
		)

INCLUDE = \
	-I/../
		
LINK = \
	${USR_LIB_DIR}/mysql ${LDOPT} -lmysqlclient \
	${USR_LIB_DIR} -lz -rdynamic -ldl  -lpthread  -lprotobuf  -lrt -lcurl

##################################下面的基本不用修改#############################

#屏蔽就是 release 编译
DEBUG = yes

ifeq "$(DEBUG)" "yes"
	VER_INFO = --- This is a debug version!! ---
	ARGS_OPT = -D_DEBUG_
else
	VER_INFO = --- This is a release version!! ---
endif

O_C_FILE = $(patsubst %.c, %.o, $(C_FILE))
O_CPP_FILE = $(patsubst %.cpp, %.o, $(CPP_FILE))

machine=$(shell uname -m)
ifeq "$(machine)" "x86_64"
	USR_LIB_DIR=-L/usr/lib64
else
	USR_LIB_DIR=-L/usr/lib
endif

LDOPT=-Wl,-Bdynamic	

C_ARGS = -g -Wall -D_POSIX_MT_ $(ARGS_OPT)
	
all: $(O_C_FILE) $(O_CPP_FILE) $(TARGET_BIN)


$(O_C_FILE) : $(C_FILE)
	ccache g++ -o $@ -c $(patsubst %.o, %.c, $@) $(INCLUDE) $(C_ARGS)

$(O_CPP_FILE) : $(CPP_FILE)
	ccache g++ -o $@ -c $(patsubst %.o, %.cpp, $@) $(INCLUDE) $(C_ARGS)

$(TARGET_BIN): $(O_CPP_FILE) $(O_C_FILE)
	$(CXX) -o $(TARGET_BIN) -Wl,-Bstatic $(O_CPP_FILE)$(O_C_FILE) $(LINK)
	
	@echo $(VER_INFO)
	@echo done
	
clean:
	rm -f $(O_C_FILE) $(O_CPP_FILE) $(TARGET_BIN)








