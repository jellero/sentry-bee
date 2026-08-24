# References

## Components

- ST LIS2DW12 product page: https://www.st.com/en/mems-and-sensors/lis2dw12.html
- ST LIS2DW12 application note AN5038: https://www.st.com/resource/en/application_note/dm00401877-lis2dw12-alwayson-3d-accelerometer-stmicroelectronics.pdf
- Sensirion SHT4x datasheet: https://sensirion.com/resource/datasheet/sht4x
- SIMCom SIM7672X product page: https://www.simcom.com/product/SIM7672.html
- SIMCom SIM7672 technical files (AT command manual and hardware design): https://www.simcom.com/technical_files-p1.html?filetype=0&pro_cat=0&pro_li=157&time=0

## Biological / signal rationale

These sources motivate the sensor choices but do **not** provide production-ready thresholds for every apiary.

- Ramsey et al., vibration-based monitoring and discrimination of swarming intent, Scientific Reports (2020): https://www.nature.com/articles/s41598-020-66115-5
- Bencsik et al., honeybee colony vibration and swarming-related signatures, Computers and Electronics in Agriculture: https://www.sciencedirect.com/science/article/pii/S0168169911000068
- Temperature-based observation of honeybee swarming behaviour: https://www.sciencedirect.com/science/article/pii/S1537511016300964
- Acoustic changes associated with queen removal / queenless state: https://www.mdpi.com/2079-7737/12/11/1392

## Engineering interpretation

Sentry-Bee deliberately uses these papers only to choose useful sensing modalities and initial frequency ranges. Classifier thresholds, persistence rules and confidence scores must be validated with labelled field data collected from the actual hive design, mounting geometry, climate, season and bee population being monitored.
