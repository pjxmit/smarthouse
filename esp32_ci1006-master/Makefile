#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

${shell git log -1   | sed -n '1p' | sed 's/^/"/' | sed 's/$$/";/' > ./components/DeviceModules/git_version.git}
PROJECT_NAME := app

include $(IDF_PATH)/make/project.mk

