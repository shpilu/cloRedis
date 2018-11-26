//
//

#ifndef CLORIS_CLOREDIS_MACRO_H_
#define CLORIS_CLOREDIS_MACRO_H_

namespace cloris {

enum ERR_STATE {
    STATE_OK = 0,
    STATE_ERROR_TYPE = 1,
    STATE_ERROR_COMMAND = 2,
    STATE_ERROR_HIREDIS = 3,
    STATE_ERROR_INVOKE = 4,
};

} // namespace cloris

#endif // CLORIS_CLOREDIS_MACRO_H_
