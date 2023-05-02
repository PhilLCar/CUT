#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN
#error UNIMPLEMENTED
#else

typedef struct userinfo {
  char name[256];
  char email[256];
  char editor[256];
} UserInfo;

void getDate(char *date, int full) {
  FILE *datefile = popen(full ? "date +\"%Y-%m-%d %H:%M:%S\"" : "date +%F", "r");
  for (int i = 0; ; i++) {
    char c = fgetc(datefile);
    if (c == '\n' || c == EOF) {
      date[i] = 0;
      break;
    }
    date[i] = c;
  }
  fclose(datefile);
}
#endif

int parseVersionNumber(FILE *file) {
  char c;
  char v[32];
  int  i = 0;
  while ((c = fgetc(file)) != EOF && c != ' '); // #define
  while ((c = fgetc(file)) != EOF && c != ' '); // MACRO_NAME
  while ((c = fgetc(file)) != EOF && c != '\n') v[i++] = c;
  v[i] = 0;
  return atoi(v);
}

UserInfo *getUserInfo() {
  FILE     *config = popen("git config --list", "r");
  UserInfo *info   = malloc(sizeof(UserInfo));
  char     *name   = "user.name=";
  char     *email  = "user.email=";
  char     *editor = "core.editor=";

  memset(info->name,   0, sizeof(info->name));
  memset(info->email,  0, sizeof(info->email));
  memset(info->editor, 0, sizeof(info->editor));
  if (config) {
    char c;
    while ((c = fgetc(config)) != EOF && (!info->name[0] || !info->email[0] || !info->editor[0])) {
      int n = 1, e = 1, d = 1;
      for (int i = 0; n || e || d ; i++) {
        if (n && !name[i]) {
          for (int j = 0; c != '\n' && c != EOF; j++) {
            info->name[j] = c;
            c = fgetc(config);
          }
          break;
        } else if (e && !email[i]) {
          for (int j = 0; c != '\n' && c != EOF; j++) {
            info->email[j] = c;
            c = fgetc(config);
          }
          break;
        } else if (d && !editor[i]) {
          for (int j = 0; c != '\n' && c != EOF; j++) {
            info->editor[j] = c;
            c = fgetc(config);
          }
          break;
        }
        if (n && name[i]   != c) n = 0;
        if (e && email[i]  != c) e = 0;
        if (d && editor[i] != c) d = 0;
        c = fgetc(config);
      }
      while (c != '\n' && c != EOF) c = fgetc(config);
    }
    fclose(config);
  }

  return info;
}

int getDescription(char *version) {
  int       data = 0;
  char      filename[128];
  char      command[512];
  FILE     *description;
  UserInfo *ui = getUserInfo();

  sprintf(filename, "misc/version/description/%s.ver", version);
  description = fopen(filename, "w+");

  if (description) {
    char date[64];
    getDate(date, 1);

    fprintf(description, "\n");
    fprintf(description, "# Describe briefly the new features introduced in this build, the bug fixes, etc.\n");
    fprintf(description, "# Build author name:  %s\n", ui->name);
    fprintf(description, "# Build author email: %s\n", ui->email);
    fprintf(description, "# Build verison:      %s\n", version);
    fprintf(description, "# Build time:         %s\n", date);
    fclose(description);

    sprintf(command, "%s %s", ui->editor, filename);
    system(command);

    description = fopen(filename, "r");
    if (description) {
      if (fgetc(description) != '\n') {
        data = 1;
      }
      fclose(description);
    }
  }

  if (ui) free(ui);
  return !data;
}

int makeBuild(char *version) {
  int  success = 0;
  char command[512];
  system("make clean");
  if (!system("make cisor")) {
    sprintf(command, "make unit-test output=misc/version/description/%s.ver", version);
    success = !system(command);
  }
  if (!success) {
    char c;
    printf("The build failed, do you wish to commit/push your changes anyway? [y/N]");
    c = getchar();
    if (c == 'Y' || c == 'y') success = 1;
  }
  return success;
}

void gitUpdate(char *version) {
  char command[512];

  system("git add .");
  sprintf(command, "git commit -F misc/version/description/%s.ver", version);
  system(command);
  sprintf(command, "git tag -a %s $(git log --format=\"%%H\" -n 1) -m \"Version %s\"", version, version);
  system(command);
  system("git push");
  sprintf(command, "git push origin %s", version);
  system(command);
}

void updateVersion(int level) {
  FILE *version = fopen("cisor/inc/version.h", "r");
  int   major, minor, revision, build;
  
  if (version) {
    char vstring[128];

    major    = parseVersionNumber(version);
    minor    = parseVersionNumber(version);
    revision = parseVersionNumber(version);
    build    = parseVersionNumber(version);

    fclose(version);

    switch (level) {
    case 3:
      major++;
      minor = revision = build = 0;
      break;
    case 2:
      minor++;
      revision = build = 0;
      break;
    case 1:
      revision++;
      build = 0;
      break;
    case 0:
    default:
      build++;
      break;
    }

    sprintf(vstring, "%d.%d.%d.%d", major, minor, revision, build);
    if (getDescription(vstring)) return;

    version = fopen("cisor/inc/version.h", "w");
    if (version) {
      char date[32];
      getDate(date, 0);

      fprintf(version, "#define VERSION_MAJOR    %d\n", major);
      fprintf(version, "#define VERSION_MINOR    %d\n", minor);
      fprintf(version, "#define VERSION_REVISION %d\n", revision);
      fprintf(version, "#define VERSION_BUILD    %d\n", build);
      fprintf(version, "\n");
      fprintf(version, "#define BUILD_DATE       \"%s\"\n", date);

      fclose(version);

      if (makeBuild(vstring)) gitUpdate(vstring);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc == 2 && !strcmp(argv[1], "revision")) {
    updateVersion(1);
  } else if (argc == 2 && !strcmp(argv[1], "minor")) {
    updateVersion(2);
  } else if (argc == 2 && !strcmp(argv[1], "major")) {
    updateVersion(3);
  } else {
    updateVersion(0);
  }
}