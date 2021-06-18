# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${BUILDDIR}/${CONF}

# Object Files
OBJECTFILES= \
        ${OBJECTDIR}/Source/Abstractions.o \
        ${OBJECTDIR}/Source/Controller.o \
        ${OBJECTDIR}/Source/Inputs.o \
        ${OBJECTDIR}/Source/Manager.o \
        ${OBJECTDIR}/Source/MathSupport.o \
        ${OBJECTDIR}/Source/Planner.o \
        ${OBJECTDIR}/Source/Sensors.o \
        ${OBJECTDIR}/Source/main.o

BALLOONOBJ=${OBJECTDIR}/Balloon/Balloon.o

# C Compiler Flags; Used for Balloon
CFLAGS=-O2 -fopenmp

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=-DDEBUG -g -IInclude -std=c++14

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: .balloon-build
	"${MAKE}"  -f Makefile-${CONF}.mk ${DISTDIR}/${CONF}/${PROJECTNAME}

${DISTDIR}/${CONF}/${PROJECTNAME}: ${OBJECTFILES}
	${MKDIR} -p ${DISTDIR}/${CONF}
	${LINK.cc} -o ${DISTDIR}/${CONF}/${PROJECTNAME} ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/Source/Abstractions.o: Source/Abstractions.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Abstractions.o Source/Abstractions.cpp

${OBJECTDIR}/Source/Controller.o: Source/Controller.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Controller.o Source/Controller.cpp

${OBJECTDIR}/Source/Inputs.o: Source/Inputs.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Inputs.o Source/Inputs.cpp

${OBJECTDIR}/Source/Manager.o: Source/Manager.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Manager.o Source/Manager.cpp

${OBJECTDIR}/Source/MathSupport.o: Source/MathSupport.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/MathSupport.o Source/MathSupport.cpp

${OBJECTDIR}/Source/Planner.o: Source/Planner.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Planner.o Source/Planner.cpp

${OBJECTDIR}/Source/Sensors.o: Source/Sensors.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Sensors.o Source/Sensors.cpp

${OBJECTDIR}/Source/main.o: Source/main.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/main.o Source/main.cpp

# Clean Targets
.clean-conf: 
	${RM} -r ${BUILDDIR}/${CONF}

.balloon-build:
	"${MAKE}"  -f Makefile-${CONF}.mk ${DISTDIR}/${CONF}/BALLOON

${DISTDIR}/${CONF}/BALLOON: ${BALLOONOBJ}
	${MKDIR} -p ${DISTDIR}/${CONF}
	${LINK.c} -o ${DISTDIR}/${CONF}/Balloon ${BALLOONOBJ} ${LDLIBSOPTIONS}


${BALLOONOBJ}: ${BALLOONDIR}/Balloon.c
	${MKDIR} -p ${OBJECTDIR}/Balloon
	${RM} "$@.d"
	$(COMPILE.c) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Balloon/Balloon.o ${BALLOONDIR}/Balloon.c

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
           
