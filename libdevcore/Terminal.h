#pragma once

namespace dev
{
namespace con
{

#if defined(_WIN32)

#define BrcReset ""       // Text Reset

#define BrcReset ""       // Text Reset

	// Regular Colors
#define BrcBlack	""    // Black
#define BrcCoal		""    // Black
#define BrcGray		""    // White
#define BrcWhite	""    // White
#define BrcMaroon	""    // Red
#define BrcRed		""    // Red
#define BrcGreen	""    // Green
#define BrcLime		""    // Green
#define BrcOrange	""    // Yellow
#define BrcYellow	""    // Yellow
#define BrcNavy		""    // Blue
#define BrcBlue		""    // Blue
#define BrcViolet	""    // Purple
#define BrcPurple	""    // Purple
#define BrcTeal		""    // Cyan
#define BrcCyan		""    // Cyan

#define BrcBlackBold	""      // Black
#define BrcCoalBold		""      // Black
#define BrcGrayBold		""      // White
#define BrcWhiteBold	""      // White
#define BrcMaroonBold	""      // Red
#define BrcRedBold		""      // Red
#define BrcGreenBold	""      // Green
#define BrcLimeBold		""      // Green
#define BrcOrangeBold	""      // Yellow
#define BrcYellowBold	""      // Yellow
#define BrcNavyBold		""      // Blue
#define BrcBlueBold		""      // Blue
#define BrcVioletBold	""      // Purple
#define BrcPurpleBold	""      // Purple
#define BrcTealBold		""      // Cyan
#define BrcCyanBold		""      // Cyan

	// Background
#define BrcOnBlack		""       // Black
#define BrcOnCoal		""		 // Black
#define BrcOnGray		""       // White
#define BrcOnWhite		""		 // White
#define BrcOnMaroon		""       // Red
#define BrcOnRed		""       // Red
#define BrcOnGreen		""       // Green
#define BrcOnLime		""		 // Green
#define BrcOnOrange		""       // Yellow
#define BrcOnYellow		""		 // Yellow
#define BrcOnNavy		""       // Blue
#define BrcOnBlue		""		 // Blue
#define BrcOnViolet		""       // Purple
#define BrcOnPurple		""		 // Purple
#define BrcOnTeal		""       // Cyan
#define BrcOnCyan		""		 // Cyan

	// Underline
#define BrcBlackUnder	""       // Black
#define BrcGrayUnder	""       // White
#define BrcMaroonUnder	""       // Red
#define BrcGreenUnder	""       // Green
#define BrcOrangeUnder	""       // Yellow
#define BrcNavyUnder	""       // Blue
#define BrcVioletUnder	""       // Purple
#define BrcTealUnder	""       // Cyan

#else

#define BrcReset "\x1b[0m"       // Text Reset

// Regular Colors
#define BrcBlack "\x1b[30m"        // Black
#define BrcCoal "\x1b[90m"       // Black
#define BrcGray "\x1b[37m"        // White
#define BrcWhite "\x1b[97m"       // White
#define BrcMaroon "\x1b[31m"          // Red
#define BrcRed "\x1b[91m"         // Red
#define BrcGreen "\x1b[32m"        // Green
#define BrcLime "\x1b[92m"       // Green
#define BrcOrange "\x1b[33m"       // Yellow
#define BrcYellow "\x1b[93m"      // Yellow
#define BrcNavy "\x1b[34m"         // Blue
#define BrcBlue "\x1b[94m"        // Blue
#define BrcViolet "\x1b[35m"       // Purple
#define BrcPurple "\x1b[95m"      // Purple
#define BrcTeal "\x1b[36m"         // Cyan
#define BrcCyan "\x1b[96m"        // Cyan

#define BrcBlackBold "\x1b[1;30m"       // Black
#define BrcCoalBold "\x1b[1;90m"      // Black
#define BrcGrayBold "\x1b[1;37m"       // White
#define BrcWhiteBold "\x1b[1;97m"      // White
#define BrcMaroonBold "\x1b[1;31m"         // Red
#define BrcRedBold "\x1b[1;91m"        // Red
#define BrcGreenBold "\x1b[1;32m"       // Green
#define BrcLimeBold "\x1b[1;92m"      // Green
#define BrcOrangeBold "\x1b[1;33m"      // Yellow
#define BrcYellowBold "\x1b[1;93m"     // Yellow
#define BrcNavyBold "\x1b[1;34m"        // Blue
#define BrcBlueBold "\x1b[1;94m"       // Blue
#define BrcVioletBold "\x1b[1;35m"      // Purple
#define BrcPurpleBold "\x1b[1;95m"     // Purple
#define BrcTealBold "\x1b[1;36m"        // Cyan
#define BrcCyanBold "\x1b[1;96m"       // Cyan

// Background
#define BrcOnBlack "\x1b[40m"       // Black
#define BrcOnCoal "\x1b[100m"   // Black
#define BrcOnGray "\x1b[47m"       // White
#define BrcOnWhite "\x1b[107m"   // White
#define BrcOnMaroon "\x1b[41m"         // Red
#define BrcOnRed "\x1b[101m"     // Red
#define BrcOnGreen "\x1b[42m"       // Green
#define BrcOnLime "\x1b[102m"   // Green
#define BrcOnOrange "\x1b[43m"      // Yellow
#define BrcOnYellow "\x1b[103m"  // Yellow
#define BrcOnNavy "\x1b[44m"        // Blue
#define BrcOnBlue "\x1b[104m"    // Blue
#define BrcOnViolet "\x1b[45m"      // Purple
#define BrcOnPurple "\x1b[105m"  // Purple
#define BrcOnTeal "\x1b[46m"        // Cyan
#define BrcOnCyan "\x1b[106m"    // Cyan

// Underline
#define BrcBlackUnder "\x1b[4;30m"       // Black
#define BrcGrayUnder "\x1b[4;37m"       // White
#define BrcMaroonUnder "\x1b[4;31m"      // Red
#define BrcGreenUnder "\x1b[4;32m"       // Green
#define BrcOrangeUnder "\x1b[4;33m"      // Yellow
#define BrcNavyUnder "\x1b[4;34m"        // Blue
#define BrcVioletUnder "\x1b[4;35m"      // Purple
#define BrcTealUnder "\x1b[4;36m"        // Cyan

#endif

}

}
