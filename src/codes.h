#ifndef CODES_H
#define CODES_H

// Dependency codes (codeId) for the Dependency Evaluation Engine (dee.h). Registered against
// a context -- see dee.h's worldContext()/factionContext()/cityContext() -- via
// DependencyEvaluationEngine::regDep(contextId, codeId).

// Vikings (faction 0) codes.
#define VIKINGS_CAN_BUILD_TRIREME (0x0 + 0x1)

#endif // CODES_H
