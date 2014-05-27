

#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

#include "boost/filesystem.hpp"

#include "rfs-calls.h"

namespace rfs_examples {

    void pull_tree( vtrc::client::vtrc_client_sptr &client,
                interfaces::remote_fs          &impl,
                const std::string              &local_path,
                const std::string              &remote_path )
    {

    }

}
