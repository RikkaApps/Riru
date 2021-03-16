#ifndef _MAIN_H
#define _MAIN_H

namespace Entry {

    void Unload(jboolean is_child_zygote);

    bool IsSelfUnloadAllowed();

}
#endif // _MAIN_H
