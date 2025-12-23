package com.pragmatsoft.faf.di

import android.content.Context
import com.pragmatsoft.faf.data.local.DataStoreRepository
import com.pragmatsoft.faf.utils.AndroidStringProvider
import com.pragmatsoft.faf.utils.AudioHelper
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
class CommonModule {
    @Provides
    @Singleton
    fun provideTasksRepository(@ApplicationContext context: Context): DataStoreRepository {
        return DataStoreRepository(context)
    }

    @Provides
    @Singleton
    fun provideAudioHelper(@ApplicationContext context: Context): AudioHelper {
        return AudioHelper(context)
    }

    @Provides
    @Singleton
    fun provideAndroidStringProvider(@ApplicationContext context: Context): AndroidStringProvider {
        return AndroidStringProvider(context)
    }
}