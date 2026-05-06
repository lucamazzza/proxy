#ifndef FRONTENDCONFIG_H
#define FRONTENDCONFIG_H

#include <QString>

#include "../appcomm/model.h"

namespace frontend {

QString defaultFrontendConfigPath();
bool loadFrontendConfig(appcomm::model::AppCommConfig *config, QString *errorMessage);
bool saveFrontendConfig(const appcomm::model::AppCommConfig &config, QString *errorMessage);

} // namespace frontend

#endif // FRONTENDCONFIG_H
