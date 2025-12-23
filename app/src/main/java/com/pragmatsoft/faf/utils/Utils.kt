package com.pragmatsoft.faf.utils

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.foundation.shape.CornerBasedShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.MaterialTheme.motionScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.coerceAtLeast
import androidx.compose.ui.unit.dp

@Composable
@OptIn(ExperimentalMaterial3ExpressiveApi::class)
fun animateShapeAsState(shape: CornerBasedShape): CornerBasedShape {
    val density = LocalDensity.current

    val topStart by animateDpAsState(
        shape.topStart.toDp(density),
        motionScheme.fastSpatialSpec()
    )
    val topEnd by animateDpAsState(
        shape.topEnd.toDp(density),
        motionScheme.fastSpatialSpec()
    )
    val bottomStart by animateDpAsState(
        shape.bottomStart.toDp(density),
        motionScheme.fastSpatialSpec()
    )
    val bottomEnd by animateDpAsState(
        shape.bottomEnd.toDp(density),
        motionScheme.fastSpatialSpec()
    )

    return RoundedCornerShape(
        topStart = topStart.coerceAtLeast(0.dp),
        topEnd = topEnd.coerceAtLeast(0.dp),
        bottomStart = bottomStart.coerceAtLeast(0.dp),
        bottomEnd = bottomEnd.coerceAtLeast(0.dp),
    )
}