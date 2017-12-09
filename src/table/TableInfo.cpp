#include <table/TableInfo.hpp>
#include <sstream>

using namespace pfabric;

std::string TableInfo::generateTypeDef() const {
  std::ostringstream os;
  bool first = true;
  os << "TuplePtr<";
  for (auto& col : mColumns) {
    if (first)
      first = false;
    else
      os << ", ";
    os << col.getType();
  }
  os << ">";
  return os.str();
}

std::string TableInfo::typeSignature() const {
  std::ostringstream os;
  os << "[";
  for (auto& col : mColumns) {
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
      case ColumnInfo::UInt_Type:
        os << "u";
        break;
    }
  }
  os << "]";
  return os.str();
}

int TableInfo::findColumnByName(const std::string& colName) const {
  for (std::size_t i = 0; i < mColumns.size(); i++) {
    if (mColumns[i].getName() == colName) return (int)i;
  }
  return -1;
}

std::ostream& operator<<(std::ostream& os, pfabric::ColumnInfo::ColumnType ct) {
  using namespace pfabric;

  switch (ct) {
    case ColumnInfo::Void_Type:
      os << "";
      break;
    case ColumnInfo::Int_Type:
      os << "int";
      break;
    case ColumnInfo::Double_Type:
      os << "double";
      break;
    case ColumnInfo::String_Type:
      os << "std::string";
      break;
    case ColumnInfo::UInt_Type:
      os << "unsigned int";
      break;
  }
  return os;
}
