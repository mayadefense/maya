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
        ${OBJECTDIR}/Source/SystemStatus.o \
        ${OBJECTDIR}/Source/main.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: 
	"${MAKE}"  -f Makefile-${CONF}.mk ${DISTDIR}/${CONF}/${PROJECTNAME}

${DISTDIR}/${CONF}/${PROJECTNAME}: ${OBJECTFILES}
	${MKDIR} -p ${DISTDIR}/${CONF}
	${LINK.cc} -o ${DISTDIR}/${CONF}/${PROJECTNAME} ${OBJECTFILES} ${LDLIBSOPTIONS}


${OBJECTDIR}/Source/Abstractions.o: Source/Abstractions.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Abstractions.o Source/Abstractions.cpp

${OBJECTDIR}/Source/Controller.o: Source/Controller.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Controller.o Source/Controller.cpp

${OBJECTDIR}/Source/Inputs.o: Source/Inputs.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Inputs.o Source/Inputs.cpp

${OBJECTDIR}/Source/Manager.o: Source/Manager.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Manager.o Source/Manager.cpp

${OBJECTDIR}/Source/MathSupport.o: Source/MathSupport.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/MathSupport.o Source/MathSupport.cpp

${OBJECTDIR}/Source/Planner.o: Source/Planner.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Planner.o Source/Planner.cpp

${OBJECTDIR}/Source/Sensors.o: Source/Sensors.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/Sensors.o Source/Sensors.cpp


${OBJECTDIR}/Source/SystemStatus.o: Source/SystemStatus.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/SystemStatus.o Source/SystemStatus.cpp

${OBJECTDIR}/Source/main.o: Source/main.cpp
	${MKDIR} -p ${OBJECTDIR}/Source
	${RM} "$@.d"
	$(COMPILE.cc) -g -IInclude -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Source/main.o Source/main.cpp

# Clean Targets
.clean-conf: 
	${RM} -r ${BUILDDIR}/${CONF}

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
           
