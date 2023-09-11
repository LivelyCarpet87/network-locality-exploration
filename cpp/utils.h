#pragma once

#include "edgelist.h"
#include "network_metrics.h"

edgelist edgelist_from_file(bool weighted, std::string filepath);
