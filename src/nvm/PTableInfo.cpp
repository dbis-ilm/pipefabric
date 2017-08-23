#include "PTableInfo.hpp"

#include <sstream>

using namespace pfabric::nvm;

std::string PTableInfo::generateTypeDef() const {
  std::ostringstream os;
  bool first = true;
  os << "TuplePtr<Tuple<";
  for (auto& col : *mColumns) {
    if (first)
      first = false;
    else
      os << ", ";
    os << col.getType();
  }
  os << ">>";
  return os.str();
}

std::string PTableInfo::typeSignature() const {
  std::ostringstream os;
  os << "[";
  for (auto& col : *mColumns) {
    switch (col.getType()) {
      case ColumnInfo::Void_Type:
        os << "V";
        break;
      case ColumnInfo::Int_Type:
        os << "i";
        break;
      case ColumnInfo::Double_Type:
        os << "d";
        break;
      case ColumnInfo::String_Type:
        os << "S";
        break;
    }
  }
  os << "]";
  return os.str();
}

int PTableInfo::findColumnByName(const std::string& colName) const {
  for (std::size_t i = 0; i < (*mColumns).size(); i++) {
    if (mColumns->at(i).getName() == colName) return (int)i;
  }
  return -1;
}