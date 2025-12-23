package com.pragmatsoft.faf.ui.screens.main.components.permissions

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.rounded.ArrowForward
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Text
import androidx.compose.material3.rememberModalBottomSheetState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.LifecycleResumeEffect
import com.pragmatsoft.faf.R
import com.pragmatsoft.faf.ui.theme.FAFTheme
import com.pragmatsoft.faf.ui.theme.ShapeDefaults.bottomListItemShape
import com.pragmatsoft.faf.ui.theme.ShapeDefaults.cardShape
import com.pragmatsoft.faf.ui.theme.ShapeDefaults.middleListItemShape
import com.pragmatsoft.faf.ui.theme.ShapeDefaults.topListItemShape
import com.pragmatsoft.faf.utils.animateShapeAsState
import com.pragmatsoft.faf.utils.rememberBatteryPermissionHandler
import com.pragmatsoft.faf.utils.rememberMicPermissionHandler
import com.pragmatsoft.faf.utils.rememberNotificationPermissionHandler

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PermissionsModalBottomSheet(onCloseRequest: () -> Unit) {
    val bottomSheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true)

    ModalBottomSheet(
        onDismissRequest = onCloseRequest,
        sheetState = bottomSheetState
    ) {
        PermissionsView(onAllPermissionsGranted = onCloseRequest)
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun PermissionsView(onAllPermissionsGranted: () -> Unit) {
    val (isMicPermissionGranted, requestMicPermission) = rememberMicPermissionHandler { }
    val (isNotificationPermissionGranted, requestNotificationPermission) = rememberNotificationPermissionHandler()
    val (isBatteryPermissionGranted, requestBatteryPermission) = rememberBatteryPermissionHandler()

    LifecycleResumeEffect(
        isMicPermissionGranted,
        isNotificationPermissionGranted,
        isBatteryPermissionGranted
        ) {
        if (isMicPermissionGranted
            && isNotificationPermissionGranted
            && isBatteryPermissionGranted) {
            onAllPermissionsGranted()
        }

        onPauseOrDispose { }
    }

    val items = listOf(
        Triple(
            isMicPermissionGranted,
            stringResource(R.string.mic_using),
            requestMicPermission
        ),
        Triple(
            isNotificationPermissionGranted,
            stringResource(R.string.notification_show),
            requestNotificationPermission
        ),
        Triple(
            isBatteryPermissionGranted,
            stringResource(R.string.battery_optimization),
            requestBatteryPermission
        )
    )
        .filter { !it.first }

    Column(
        modifier = Modifier
            .padding(start = 16.dp, end = 16.dp, bottom = 16.dp)
    ) {
        Text(
            modifier = Modifier.padding(bottom = 30.dp),
            text = stringResource(R.string.permissions),
            style = MaterialTheme.typography.headlineSmall
        )

        Text(
            modifier = Modifier.padding(bottom = 10.dp),
            text = stringResource(R.string.permissions_description),
        )

        Column(
            verticalArrangement = Arrangement.spacedBy(2.dp)
        ) {
            items.mapIndexed { index, item ->
                PermissionItemView(
                    item = item,
                    isFirst = index == 0,
                    isLast = index == items.size - 1,
                    isSingle = items.size == 1
                )
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun PermissionItemView(
    item: Triple<Boolean, String, () -> Unit>,
    isFirst: Boolean,
    isLast: Boolean,
    isSingle: Boolean
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isPressed by interactionSource.collectIsPressedAsState()

    val shape = if (isPressed) {
        cardShape
    } else if (isSingle) {
        cardShape
    }  else if (isFirst) {
        topListItemShape
    } else if (isLast) {
        bottomListItemShape
    } else {
        middleListItemShape
    }

    val animatedShape = animateShapeAsState(shape)

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(
                onClick = {
                    item.third()
                },
                interactionSource = interactionSource,
                indication = null
            )
            .clip(animatedShape)
            .background(MaterialTheme.colorScheme.surfaceContainerHighest)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 20.dp, horizontal = 16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(text = item.second)
            Icon(
                imageVector = Icons.AutoMirrored.Rounded.ArrowForward,
                contentDescription = "go"
            )
        }
    }
}

@Preview()
@Composable
fun PermissionsViewPreview() {
    PermissionsViewPreview(false)
}

@Preview()
@Composable
fun PermissionsViewNightPreview() {
    PermissionsViewPreview(true)
}

@Composable
fun PermissionsViewPreview(darkTheme: Boolean) {
    FAFTheme(
        dynamicColor = false,
        darkTheme = darkTheme
    ) {
        Box(
            modifier = Modifier
                .background(MaterialTheme.colorScheme.background)
        ) {
            PermissionsView(onAllPermissionsGranted = { })
        }
    }
}