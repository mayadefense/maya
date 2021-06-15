# This code depends on make tool being used
DEPFILES=$(wildcard $(addsuffix .d, ${OBJECTFILES} ${BALLOONOBJ} ${TESTOBJECTFILES}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
