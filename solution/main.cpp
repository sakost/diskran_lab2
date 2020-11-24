#include "patricia.h"

#include <cstring>
#include <fstream>
#include <iostream>

void KeyToLower(char *key) {
  for (std::size_t i = 0; i < strlen(key); ++i) {
    key[i] = (char)tolower(key[i]);
  }
}

const int BUFFER_SIZE = 1024 * 1024;

int main() {
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::cout.tie(nullptr);

  std::ofstream fout;
  std::ifstream fin;

  auto *patricia = new TPatricia();

  char cmd[KEY_LENGTH + 1];

#ifdef ONPC
  std::size_t index = 0;
#endif

  while (std::cin >> cmd) {
#ifdef ONPC
    index++;
#endif
    if (!std::strcmp(cmd, "+")) {
      std::cin >> cmd;
      TValue val;
      std::cin >> val;
      KeyToLower(cmd);
      if (patricia->AddItem(cmd, val) != nullptr)
        std::cout << "OK" << std::endl;
      else
        std::cout << "Exist" << std::endl;

    } else if (!std::strcmp(cmd, "-")) {
      std::cin >> cmd;
      KeyToLower(cmd);

      if (patricia->Erase(cmd))
        std::cout << "OK" << std::endl;
      else
        std::cout << "NoSuchWord" << std::endl;

    } else if (!std::strcmp(cmd, "!")) {
      std::cin >> cmd;
      if (!strcmp(cmd, "Save")) {
        std::cin >> cmd;
        fout.open(cmd, std::ios::out | std::ios::binary | std::ios::trunc);

        patricia->Serialize(fout);

        fout.close();
        std::cout << "OK" << std::endl;
      } else if (!strcmp(cmd, "Load")) {
        std::cin >> cmd;
        fin.open(cmd, std::ios::in | std::ios::binary);

        delete patricia;
        patricia = new TPatricia();
        patricia->Deserialize(fin);

        fin.close();
        std::cout << "OK" << std::endl;
      } else {
        std::cout << "ERROR: no such option" << std::endl;
      }
    } else {
      KeyToLower(cmd);

      TNode *res = patricia->Find(cmd);
      if (res != nullptr)
        std::cout << "OK: " << res->Value << std::endl;
      else
        std::cout << "NoSuchWord" << std::endl;
    }
#ifdef ONPC
    std::cerr << index << ":" << std::endl;
    patricia->TreeDebugPrint(std::cerr);
    std::cerr.flush();
#endif
  }

  delete patricia;
  return 0;
}
