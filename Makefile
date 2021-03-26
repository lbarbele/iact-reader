HESSIODIR=${HOME}/programas/hessioxxx
ROOTINC=`root-config --incdir`
CXXFLAGS+=-std=c++11 `root-config --cflags`
LDFLAGS+=-lm -ldl -rdynamic
LIBS=-lhessio++ -lCore -lHist -lRIO -lMathCore -lTree -lPhysics
OBJECTS=obj/analyzeBunches.o obj/atmosphericTransmission.o obj/getInputs.o obj/getProfiles.o obj/getOptions.o obj/makeHeader.o obj/iact-reader.o

OBJDIR=obj
SRCDIR=src

iact-reader: $(OBJECTS)
	g++ ${LDFLAGS} -L`root-config --libdir` -L${HESSIODIR}/lib -fPIC -Wl,-rpath=${HESSIODIR}/lib ${LIBS} $(OBJECTS) -o iact-reader

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p obj
	g++ -c $(CXXFLAGS) -I${HESSIODIR}/include -I./headers -fPIC -o $@ $<

clean:
	rm -rf obj
	rm -f iact-reader
	
.PHONY: clean
