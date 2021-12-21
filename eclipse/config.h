/*
 * config.h
 *
 *  Created on: 21 Dec 2021
 *      Author: Jannes Eindhoven
 */

#ifndef ECLIPSE_CONFIG_H_
#define ECLIPSE_CONFIG_H_

// Especially for Eclipse, where it uses internal builder
// JCE< 21-12-2021

// template file for cmake to make config.h

#define version_major 1
#define version_minor 0
#define version_patch 0

#define version_str "eclipse_" #version_major "." #version_minor "." #version_match

// These come from the project config.
//#cmakedefine HAVE_S7
//#cmakedefine HAVE_RPI
//#cmakedefine HAVE_USB
//#cmakedefine HAVE_MARIA





#endif /* ECLIPSE_CONFIG_H_ */
